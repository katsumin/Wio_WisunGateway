#include "WisunManager.h"
#include "debug.h"
#include <queue>
#include <set>

//
#define SEND_INTERVAL (15)
std::queue<RequestResponse> queue;
std::set<String> keys;
void WisunManager::sendTask(void *arm)
{
    WisunManager *instance = (WisunManager *)arm;
    debug_timestamp();
    debug_println("sendTask start.");
    for (int count = 0;; count++)
    {
        if (instance->getState() != CONNECT)
        {
            delay(1000);
            continue;
        }
        delay(SEND_INTERVAL * 1000);
        if (queue.empty() || count % 4 == 0) // キューが空のとき、またはキューが空でなくても4回に1回はView用の取得
        {
            // view用の要求
            uint8_t *buf = instance->getDataStore()->getDevice()->request();
            instance->udpSend(&buf[1], (size_t)buf[0]);
        }
        else
        {
            RequestResponse rr = queue.front();
            queue.pop();
            String key = rr.getKey(EL_DEOJ);
            keys.erase(key);
            instance->udpSend(rr.getFrame(), rr.getSize());
            debug_timestamp();
            debug_print("sendTask packet: ");
            uint8_t *buf = rr.getFrame();
            debug_dumpln((const char *)buf, rr.getSize(), HEX);
            debug_printfln(true, "pop from queue -> size=%d", queue.size());
        }
    }
    vTaskDelete(NULL);
    debug_timestamp();
    debug_println("sendTask stop.");
}

WisunManager::WisunManager()
{
}

WisunManager::~WisunManager()
{
}

//
void WisunManager::begin()
{
    Serial1.begin(115200);

    // 送信タスク起動
    xTaskCreate(sendTask, (const char *)"sendTask", 1024, this, tskIDLE_PRIORITY + 1, NULL);
}

//
void WisunManager::stop()
{
    Serial1.end();
}

//
void WisunManager::sendSerial(const uint8_t *buffer, size_t size)
{
    Serial1.write(buffer, size);
}

//
void WisunManager::update()
{
    int packetSize = 0;
    if (0 != (packetSize = read()))
    {
        RequestResponse rr(IPAddress((uint32_t)0), _rbuf, packetSize);
        getDataStore()->set(rr);
    }
}

void WisunManager::get(RequestResponse &rr)
{
    // 送信タスクに渡すキューに登録
    String key = rr.getKey(EL_DEOJ);
    if (keys.count(key) > 0)
    {
        debug_printfln(true, "queued:%p, size=%d", rr.getFrame(), queue.size());
    }
    else
    {
        queue.push(rr);
        keys.insert(key);
        debug_printfln(true, "push to queue -> %p, size=%d", rr.getFrame(), queue.size());
    }
}
