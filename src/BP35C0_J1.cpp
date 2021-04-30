#include "BP35C0_J1.h"
#include "debug.h"
#include <queue>

BP35C0_J1::BP35C0_J1()
{
}

BP35C0_J1::~BP35C0_J1()
{
}

//
static uint16_t sum(uint8_t *buf, uint16_t len)
{
    uint16_t sum = 0;
    for (int i = 0; i < len; i++)
        sum += (uint16_t)buf[i];
    return sum;
}

//
static void sendReq(uint8_t *cmd, uint8_t *data, uint16_t len)
{
    uint16_t l = len + 4;
    uint8_t head[12] = {UNQ_REQ, cmd[0], cmd[1]}; // ユニークコード, 要求コマンド
    head[6] = l >> 8 & 0xff;                      // メッセージ長
    head[7] = l & 0xff;
    uint16_t s = sum(head, 8);
    head[8] = s >> 8 & 0xff; // ヘッダ部チェックサム
    head[9] = s & 0xff;
    s = sum(data, len);
    head[10] = s >> 8 & 0xff; // データ部チェックサム
    head[11] = s & 0xff;
    debug_timestamp();
    debug_print("sendReq: ");
    debug_dump((const char *)head, sizeof(head), HEX);
    debug_dumpln((const char *)data, len, HEX);
    WisunManager::sendSerial(head, sizeof(head)); // ヘッダ部(12)
    if (len > 0)
        WisunManager::sendSerial(data, len); // データ部(len)
}

//
typedef struct
{
    uint16_t cmd;
    uint16_t len;
    uint8_t *data;
} BP35C0_J1_RES;
#define RCV_IDLE (0)
#define RCV_HEADER (1)
#define RCV_DATA (2)
#define QUEUE_DEPTH 5
xQueueHandle qh = nullptr;
void BP35C0_J1::receiveTask(void *arm)
{
    qh = xQueueCreate(QUEUE_DEPTH, sizeof(BP35C0_J1_RES));
    debug_printfln(true, "receiveTask start. xQueueHandle(%p)", qh);
    uint32_t uniq = 0;
    uint16_t pos = 0;
    uint16_t len = 0;
    uint16_t sum_d = 0;
    uint8_t head[8];
    byte state = RCV_IDLE;
    uint16_t cmd;
    uint8_t *data;
    while (true)
    {
        if (Serial1.available() > 0)
        {
            int d = Serial1.read();
            switch (state)
            {
            case RCV_IDLE:
            {
                uniq <<= 8;
                uniq |= d & 0xff;
                if (uniq == UNQ_RES)
                {
                    debug_printf(true, "RCV_IDLE: %04x\t", uniq);
                    state = RCV_HEADER;
                    pos = 0;
                }
            }
            break;
            case RCV_HEADER:
            {
                head[pos++] = d;
                if (pos == 8)
                {
                    debug_print("RCV_HEADER: ");
                    debug_dump((const char *)head, sizeof(head), HEX);
                    debug_print("\t");
                    uint16_t sum_h = (UNQ_RES >> 24 & 0xff)   //
                                     + (UNQ_RES >> 16 & 0xff) //
                                     + (UNQ_RES >> 8 & 0xff)  //
                                     + (UNQ_RES & 0xff)       //
                                     + head[0]                //
                                     + head[1]                //
                                     + head[2]                //
                                     + head[3];               //
                    uint16_t sum_ch = head[4] << 8 | head[5];
                    if (sum_h == sum_ch)
                    {
                        cmd = head[0] << 8 | head[1];
                        len = head[2] << 8 | head[3];
                        len -= 4;
                        if (len > 0)
                        {
                            data = new uint8_t[len];
                            state = RCV_DATA;
                            sum_d = 0;
                            pos = 0;
                        }
                        else
                        {
                            BP35C0_J1_RES res;
                            res.cmd = cmd;
                            res.len = len;
                            res.data = nullptr;
                            xQueueSend(qh, &res, portMAX_DELAY);
                            state = RCV_IDLE;
                            uniq = 0;
                            debug_println();
                        }
                    }
                    else
                    {
                        debug_println("RCV_HEADER check sum error.");
                        state = RCV_IDLE;
                        uniq = 0;
                    }
                }
            }
            break;
            case RCV_DATA:
            {
                sum_d += d;
                data[pos++] = d;
                if (pos == len)
                {
                    debug_printf("RCV_DATA: %04x\t", cmd);
                    debug_dumpln((const char *)data, len, HEX);
                    uint16_t sum_cd = head[6] << 8 | head[7];
                    if (sum_d == sum_cd)
                    {
                        BP35C0_J1_RES res;
                        res.cmd = cmd;
                        res.len = len;
                        res.data = data;
                        xQueueSend(qh, &res, portMAX_DELAY);
                    }
                    else
                    {
                        delete[] data;
                        debug_println("RCV_DATA check sum error.");
                    }
                    state = RCV_IDLE;
                    uniq = 0;
                }
            }
            break;
            }
        }
        delay(10);
    }
    vQueueDelete(qh);
    vTaskDelete(NULL);
    debug_timestamp();
    debug_println("receiveTask stop.");
}

//
static boolean waitResponse(uint16_t cmd, uint8_t **data, uint16_t timeout = 3000)
{
    uint32_t pre = millis();
    uint16_t diff = 0;
    BP35C0_J1_RES res;
    while (true)
    {
        if (xQueueReceive(qh, &res, (timeout - diff) / portTICK_PERIOD_MS) == pdPASS)
        {
            *data = res.data;
            debug_printfln(true, "cmd:%04x, len:%d, data:%p", res.cmd, res.len, *data);
            if (res.cmd == cmd && res.data[0] == (uint8_t)0x01)
                return true;
        }
        diff = (uint16_t)(millis() - pre);
        if (diff > timeout)
            break;
    }
    debug_timestamp();
    debug_println("timeout.");
    return false;
}

//
static void clearRecv()
{
    xQueueReset(qh);
}

//
static boolean initialize(byte ch, uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_INITIALIZE};
    uint8_t data[] = {0x05, 0x00, ch, 0x00};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_INITIALIZE, recv_data);
}

//
static boolean activescan(uint8_t **recv_data, uint8_t **scan_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_ACTIVE_SCAN};
    // スキャン時間＝4.9s、スキャンチャンネル＝ch4-17、ParingIDあり
    // uint8_t data[1 + 4 + 1 + 8] = {0x09, 0x00, 0x03, 0xff, 0xf0, 0x01};
    uint8_t data[1 + 4 + 1 + 8] = {0x08, 0x00, 0x03, 0xff, 0xf0, 0x01};
    memcpy(&data[6], &bid[32 - 8], 8);
    sendReq(cmd, data, sizeof(data));
    // 通知パラメータ取得
    for (int i = 4; i < 18; i++)
    {
        BP35C0_J1_RES res;
        if (xQueueReceive(qh, &res, 10000 / portTICK_PERIOD_MS) == pdPASS)
        {
            if (res.cmd == INF_ACTIVE_SCAN)
            {
                if (res.data[0] == 0x00)
                {
                    *scan_data = res.data;
                    debug_printfln(true, "%p\tresult:%02x, ch:%02d, count:%02x, mac:%02x%02x%02x%02x%02x%02x%02x%02x, PAN_ID:%02x%02x, RSSI:%02x",
                                   res.data,
                                   res.data[0],                                                                                                                            // result
                                   res.data[1],                                                                                                                            // ch
                                   res.data[2],                                                                                                                            // scan count
                                   res.data[3 + 0], res.data[3 + 1], res.data[3 + 2], res.data[3 + 3], res.data[3 + 4], res.data[3 + 5], res.data[3 + 6], res.data[3 + 7], // mac
                                   res.data[11 + 0], res.data[11 + 1],                                                                                                     // panid
                                   res.data[13]);                                                                                                                          // rssi
                }
                else
                {
                    delete[] res.data;
                    debug_printfln(true, "%p\tresult:%02d, ch:%02d", res.data, res.data[0], res.data[1]);
                }
            }
            else
            {
                debug_timestamp();
                debug_println("not active scan: ");
            }
        }
        else
        {
            debug_timestamp();
            debug_println("timeout.");
            return false;
        }
    }
    if (waitResponse(RES_ACTIVE_SCAN, recv_data))
    {
        return true;
    }
    else
        return false;
}

//
static boolean set_broute(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_B_ROUTE_SET_PANA_INFO};
    uint8_t data[32 + 12];
    memcpy(&data[0], bid, 32);
    memcpy(&data[32], pwd, 12);
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_B_ROUTE_SET_PANA_INFO, recv_data);
}

//
static boolean start_broute(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_B_ROUTE_START};
    uint8_t data[] = {};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_B_ROUTE_START, recv_data);
}

//
static boolean stop_broute(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_B_ROUTE_END};
    uint8_t data[] = {};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_B_ROUTE_END, recv_data);
}

//
static boolean start_broute_pana(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_B_ROUTE_PANA_START};
    uint8_t data[] = {};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_B_ROUTE_PANA_START, recv_data);
}

//
static boolean stop_broute_pana(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_B_ROUTE_PANA_END};
    uint8_t data[] = {};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_B_ROUTE_PANA_END, recv_data, 10 * 1000);
}

//
static boolean restart_broute_pana(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_B_ROUTE_PANA_RE_AUTH};
    uint8_t data[] = {};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_B_ROUTE_PANA_RE_AUTH, recv_data, 10 * 1000);
}

//
static boolean open_udp(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_UDP_OPEN};
    uint16_t port = 3610;
    uint8_t data[] = {port >> 8, port & 0xff};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_UDP_OPEN, recv_data);
}

//
static boolean close_udp(uint8_t **recv_data)
{
    clearRecv();
    uint8_t cmd[] = {CMD_UDP_CLOSE};
    uint16_t port = 3610;
    uint8_t data[] = {port >> 8, port & 0xff};
    sendReq(cmd, data, sizeof(data));
    return waitResponse(RES_UDP_CLOSE, recv_data);
}

//
#define RESET I2S_BLCK
static void hw_reset()
// static void hw_reset(uint8_t **recv_data)
{
    digitalWrite(RESET, LOW);
    delay(100);
    digitalWrite(RESET, HIGH);
    // clearRecv();
    // uint8_t cmd[] = {CMD_HW_RESET};
    // uint8_t data[] = {};
    // sendReq(cmd, data, sizeof(data));
    // return waitResponse(INF_BOOTED, recv_data);
}

//
static boolean send_data(const uint8_t *mac, const uint8_t *send_data, uint16_t send_len, uint8_t **recv_data)
{
    uint8_t cmd[] = {CMD_SEND_DATA};
    int len = 16 + 2 + 2 + 2 + send_len; // addr(16), from port(2), to port(2), send_len(2), payload(send_len)
    uint8_t data[len];
    static uint8_t mac_upper[] = {
        0xfe,
        0x80,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };
    memcpy(&data[0], mac_upper, sizeof(mac_upper));
    memcpy(&data[8], mac, 8);
    data[8] ^= 0x02; // 先頭バイトの下位2bit目を反転
    uint16_t port = 3610;
    data[16] = port >> 8;
    data[17] = port & 0xff;
    data[18] = port >> 8;
    data[19] = port & 0xff;
    data[20] = send_len >> 8;
    data[21] = send_len & 0xff;
    memcpy(&data[22], send_data, send_len);
    sendReq(cmd, data, len);
    // return waitResponse(RES_SEND_DATA, recv_data);
    return true; // UDP受信データを引き取ってしまわないように、レスポンス受信はしない
}

//
void BP35C0_J1::connectTask(void *arm)
{
    BP35C0_J1 *instance = (BP35C0_J1 *)arm;
    debug_timestamp();
    debug_println("connectTask start.");
    while (true)
    {
        debug_timestamp();
        debug_println("connectTask suspend.");
        vTaskSuspend(NULL);
        debug_timestamp();
        debug_println("connectTask resumed.");
        uint8_t *recv_data;

        byte ch = instance->_ch;
        if (ch == DEFAULT_CH)
        {
            // 初期設定要求（0x005f）
            if (!initialize(instance->_ch, &recv_data))
            {
                debug_timestamp();
                debug_println("initialize error.");
                instance->setState(DIS_CONNECT);
                continue; // 失敗
            }
            debug_printfln(true, "initialized. %p", recv_data);
            delete[] recv_data;
            delay(200);

            // アクティブスキャン（0x0051）
            uint8_t *scan_data = nullptr;
            if (!activescan(&recv_data, &scan_data))
            {
                debug_timestamp();
                debug_println("activescan error.");
                instance->setState(DIS_CONNECT);
                continue; // 失敗
            }
            if (scan_data == nullptr)
            {
                debug_timestamp();
                debug_println("not detect smartmeter.");
                instance->setState(DIS_CONNECT);
                continue; // 失敗
            }
            ch = scan_data[1];
            delete[] scan_data;
            delete[] recv_data;
        }

        // 初期設定要求（0x005f）、アクティブスキャンで応答のあったchを指定
        if (ch != DEFAULT_CH)
        {
            if (!initialize(ch, &recv_data))
            {
                debug_timestamp();
                debug_println("initialize error.");
                instance->setState(DIS_CONNECT);
                continue; // 失敗
            }
            delete[] recv_data;
            delay(200);
        }

        // BルートPANA認証情報設定（0x0054）
        if (!set_broute(&recv_data))
        {
            debug_timestamp();
            debug_println("set_broute error.");
            instance->setState(DIS_CONNECT);
            continue; // 失敗
        }
        delete[] recv_data;
        delay(500);

        // Bルート動作開始（0x0053）
        if (!start_broute(&recv_data))
        {
            debug_timestamp();
            debug_println("start_broute error.");
            instance->setState(DIS_CONNECT);
            continue; // 失敗
        }
        instance->_ch = recv_data[1];
        instance->_pan_id = recv_data[2 + 0] << 8 | recv_data[2 + 1];
        memcpy(instance->_mac, &recv_data[4], 8);
        instance->_rssi = recv_data[4 + 8];
        delete[] recv_data;
        debug_printfln(true, "ch:%d, mac:%02x%02x%02x%02x%02x%02x%02x%02x, PAN_ID:%04x, RSSI:%d",
                       instance->_ch,                                                                                                                                          // ch
                       instance->_mac[0], instance->_mac[1], instance->_mac[2], instance->_mac[3], instance->_mac[4], instance->_mac[5], instance->_mac[6], instance->_mac[7], // mac
                       instance->_pan_id,                                                                                                                                      // pan id
                       instance->_rssi);                                                                                                                                       // rssi
        delay(200);

        // UDPポートOPEN（0x0005）
        if (!open_udp(&recv_data))
        {
            debug_timestamp();
            debug_println("open_udp error.");
            instance->setState(DIS_CONNECT);
            continue; // 失敗
        }
        delete[] recv_data;
        delay(200);

        // BルートPANA開始（0x0056）
        if (!start_broute_pana(&recv_data))
        {
            debug_timestamp();
            debug_println("start_broute_pana error.");
            instance->setState(DIS_CONNECT);
            continue; // 失敗
        }
        delete[] recv_data;

        // 接続完了
        debug_timestamp();
        debug_println("connect");
        instance->setState(CONNECT);
    }
    vTaskDelete(NULL);
    debug_timestamp();
    debug_println("connectTask stop.");
}

//
void BP35C0_J1::begin()
{
    WisunManager::begin();

    // 接続タスク起動
    xTaskCreate(connectTask, (const char *)"connectTask", 1024, this, tskIDLE_PRIORITY + 1, &_connectTaskHandle);

    // 受信タスク起動
    xTaskCreate(receiveTask, (const char *)"receiveTask", 1024, this, tskIDLE_PRIORITY + 1, NULL);

    // HWリセット
    hw_reset();
}

//
void BP35C0_J1::udpSend(const uint8_t *frame, size_t size)
{
    if (getState() == CONNECT)
    {
        // データ送信（0x0008）
        uint8_t *recv_data = nullptr;
        send_data(_mac, frame, size, &recv_data);
        if (recv_data != nullptr)
            delete[] recv_data;
    }
    else
    {
        debug_timestamp();
        debug_println("not connected.");
    }
}

void BP35C0_J1::connect()
{
    // connectTask 再開
    setState(CONNECTING);
    vTaskResume(_connectTaskHandle);
}

void BP35C0_J1::disconnect()
{
    setState(DIS_CONNECTING);

    uint8_t *recv_data = nullptr;

    // BルートPANA終了（0x0057）
    if (!stop_broute_pana(&recv_data))
    {
        debug_timestamp();
        debug_println("stop_broute_pana error.");
    }
    if (recv_data != nullptr)
        delete[] recv_data;

    // UDPポートCLOSE（0x0006）
    recv_data = nullptr;
    if (!close_udp(&recv_data))
    {
        debug_timestamp();
        debug_println("close_udp error.");
    }
    if (recv_data != nullptr)
        delete[] recv_data;

    // Bルート動作終了（0x0058）
    recv_data = nullptr;
    if (!stop_broute(&recv_data))
    {
        debug_timestamp();
        debug_println("stop_broute error.");
    }
    if (recv_data != nullptr)
        delete[] recv_data;

    // HWリセット（0x00d9）
    hw_reset();

    debug_timestamp();
    debug_println("disconnect");
    setState(DIS_CONNECT);
}

// test
// uint16_t watthour_p = 1;
// uint16_t watthour_m = 1;
// uint32_t power = 100000;
// int16_t current_r = -10000;
// int16_t current_t = 10000;
// test
int BP35C0_J1::read()
{
    if (getState() == CONNECT)
    {
        BP35C0_J1_RES res;
        int len = 0;
        int queues = uxQueueMessagesWaiting(qh);
        if (queues > 0)
        {
            if (xQueueReceive(qh, &res, portMAX_DELAY) == pdPASS)
            {
                debug_printfln(true, "cmd: %04x", res.cmd);
                if (res.cmd == INF_RECV_DATA && res.data != nullptr)
                {
                    // ヘッダ部分をオフセット
                    // 送信元IPアドレス(16)、送信元ポート(2)、送信先ポート(2)、送信元PAN-ID(2)、送信先アドレス種別(1)、暗号化(1)、RSSI(1)
                    int offset = 16 + 2 + 2 + 2 + 1 + 1 + 1;
                    len = res.data[offset] << 8 | res.data[offset + 1]; // 受信データ数(2)
                    offset += 2;
                    // debug_printfln("res.len:%d, len:%d, offset:%d", res.len, len, offset);
                    if (res.len == len + offset && len < sizeof(_rbuf))
                    {
                        // debug_printfln("copy len:%d", len);
                        // 正常なデータを受信
                        memcpy(_rbuf, &res.data[offset], len);
                    }
                    else
                        len = 0;
                    delete[] res.data;
                }
            }
            debug_printfln(true, "receive len:%d", len);
        }
        return len;
        // test
        // uint32_t m = millis();
        // if (m % 1000 == 0)
        // {
        //     power--;
        //     current_r++;
        //     current_t--;
        //     byte frame[] = {
        //         0x10,
        //         0x81,
        //         0x00,
        //         0x00,
        //         0x02,
        //         0xff,
        //         0x01,
        //         0x0e,
        //         0xf0,
        //         0x01,
        //         0x72,
        //         2, //
        //         0xe8,
        //         4,
        //         current_r >> 8 & 0xff,
        //         current_r & 0xff,
        //         current_t >> 8 & 0xff,
        //         current_t & 0xff,
        //         0xe7,
        //         4,
        //         byte(power >> 24 & 0xff),
        //         byte(power >> 16 & 0xff),
        //         byte(power >> 8 & 0xff),
        //         byte(power & 0xff), //
        //     };
        // sm.getWattHourObjPlus()->updateValues(0.1, epoch);
        // sm.getWattHourObjMinus()->updateValues(0.1, epoch);
        // sm.setPower(sm.getPower() - 1);
        // sm.setCurrentR(sm.getCurrentR() + 1);
        // sm.setCurrentT(sm.getCurrentT() - 1);
        //
        // int len = sizeof(frame);
        // memcpy(_rbuf, frame, len);
        //     return len;
        // }
    }
    else
        // 切断時は、無条件で０を返す
        return 0;
}