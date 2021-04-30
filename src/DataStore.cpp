#include "DataStore.h"
#include "EthernetManager.h"
#include "WisunManager.h"
#include "debug.h"
#include <vector>

void DataStore::get(RequestResponse &rr)
{
    std::vector<uint8_t> epc_v;
    uint8_t *b = rr.getFrame();
    debug_printfln(true, "get _echo:%p, frame:%p", _echo, b);
    _echo->extractEpc(b, epc_v);

    // ECHONET_FRAME *frame = (ECHONET_FRAME *)rr.getFrame();
    ELOBJ *targetObj = nullptr;
    boolean node_profile = false;
    uint16_t classType = b[EL_DEOJ + 0] << 8 | b[EL_DEOJ + 1];
    // Serial.printf("classType:%04x, device->getClassType:%04x", classType, _device->getClassType());
    // Serial.println();
    if (classType == 0x0ef0)
    {
        // ノードプロファイルは全プロパティがキャッシュ対象
        node_profile = true;
        targetObj = &_echo->profile;
    }
    else if (classType == _device->getClassType())
    {
        targetObj = &_echo->details;
    }
    else
    {
        debug_printfln(true, "other class:%04x", classType);
        return;
    }

    // キャッシュ対象、かつキャッシュ済みかの判定
    boolean valid = true;
    for (auto itr = epc_v.begin(); itr != epc_v.end(); itr++)
    {
        byte epc = *itr;
        if (!node_profile && _nocache.count(epc) > 0)
        {
            // ノードプロファイル以外でキャッシュ対象外のEPCを含む　⇒　WisunManagerに委譲
            // Serial.printf("no check epc %02x", epc);
            valid = false;
            break;
        }
        if ((*targetObj)[epc] == nullptr)
        {
            // 未キャッシュ　⇒ WisunManagerに委譲
            // Serial.printf("no cache epc %02x", epc);
            valid = false;
            break;
        }
    }
    if (valid)
    {
        // キャッシュ済みなので即時応答
        debug_timestamp();
        debug_println("returner");
        std::set<IPAddress> addrs = rr.getAddresses();
        for (auto itr = addrs.begin(); itr != addrs.end(); itr++)
        {
            _echo->returner(*itr, rr.getFrame());
        }
    }
    else
    {
        // EPC列をキーとして保存
        IPAddress addr = *(rr.getAddresses().begin());
        String key = rr.getKey(EL_DEOJ);
        if (_delegate.count(key) > 0)
        {
            // 委譲済み（同じEPC列）の取得要求　⇒ 取得元のIPアドレスを追加
            RequestResponse *delegated = _delegate[key];
            delegated->getAddresses().insert(addr);

            uint8_t *frame = delegated->getFrame();
            int size = delegated->getSize();
            uint32_t ts = delegated->getTimestamp();
            debug_printf(true, "delegated.(%p) epc:%s, frame(%p), timestamp:%ld, data-size: %d\t", delegated, key.c_str(), frame, ts, size);
            debug_dumpln((const char *)frame, size, HEX);
        }
        else
        {
            RequestResponse *p_rr = new RequestResponse(rr);
            _delegate[key] = p_rr;

            // WisunManagerに委譲
            uint8_t *frame = p_rr->getFrame();
            int size = p_rr->getSize();
            uint32_t ts = p_rr->getTimestamp();
            debug_printf(true, "_delegate(%p) key:%s, delegate-size: %d, frame(%p), seq:%ld, data-size: %d\t", p_rr, key.c_str(), _delegate.size(), frame, ts, size);
            debug_dumpln((const char *)frame, size, HEX);
            _wm->get(rr);
        }
    }

    return;
}

void cacheUpdate(EL *echo, uint8_t *rbuf, int eoj_pos)
{
    const byte opc = rbuf[EL_OPC];
    uint16_t classType = rbuf[eoj_pos + 0] << 8 | rbuf[eoj_pos + 1];
    byte *buf = &rbuf[EL_EPC];
    for (int i = 0; i < opc; i++)
    {
        echo->update(classType, buf);
        byte pdc = buf[1];
        buf += pdc + 1 + 1;
    }
}

void DataStore::set(RequestResponse &rr)
{
    // キャッシュを更新
    byte *rbuf = rr.getFrame();
    debug_printfln(true, "rcv frame(%p)", rbuf);

    // IPアドレスで呼び出し元を判断
    auto itr = rr.getAddresses().begin();
    uint32_t addr = (uint32_t)*itr;
    if (addr == 0)
    {
        // Serial.println("set from Wisun");
        // WisunManagerからの取得応答
        cacheUpdate(_echo, rbuf, EL_SEOJ); // SEOJ=0ef0 or 0288, DEOJ=0ef0

        // Deviceを更新（Viewの更新）
        getDevice()->parseFrame(rbuf);

        if (rbuf[EL_ESV] == EL_INF)
        {
            // INFはマルチキャストで転送
            _echo->sendMulti(rbuf, rr.getSize());
            return;
        }
        // 記録しておいたIPアドレスに返送
        String key = rr.getKey(EL_SEOJ);
        if (_delegate.count(key) > 0)
        {
            // Serial.println("by delegate.");
            // 更新済みのキャッシュに対して、取得要求しなおす
            RequestResponse *delegated_rr = _delegate[key];
            int size = delegated_rr->getSize();
            uint8_t *frame = delegated_rr->getFrame();
            uint32_t ts = delegated_rr->getTimestamp();
            std::set<IPAddress> addrs = delegated_rr->getAddresses();
            debug_printf(true, "from delegate(%p): key(%s), frame(%p), seq(%ld), data-size(%d)\t", delegated_rr, key.c_str(), frame, ts, size);
            debug_dumpln((const char *)frame, size, HEX);
            for (auto itr = addrs.begin(); itr != addrs.end(); itr++)
            {
                _echo->returner(*itr, frame);
            }

            // 委譲情報を削除
            _delegate.erase(key);
            delete delegated_rr;
        }

        // 応答のない委譲情報（委譲から60s経過）があったら、WisunManagerに再委譲
        uint32_t cur = millis();
        for (auto itr = _delegate.begin(); itr != _delegate.end(); itr++)
        {
            RequestResponse *rr_p = itr->second;
            uint32_t ts = rr_p->getTimestamp();
            int32_t diff = cur - ts;
            if (diff < 0)
                diff += INT32_MAX;
            if (diff > 60 * 1000)
            {
                uint8_t *frame = rr_p->getFrame();
                rr_p->setTimestamp(cur);
                int size = rr_p->getSize();
                debug_printf(true, "retry -> delegate-size: %d, frame(%p), timestamp:%ld->%ld, data-size: %d\t", _delegate.size(), frame, ts, cur, size);
                debug_dumpln((const char *)frame, size, HEX);
                _wm->get(*rr_p);
            }
        }
    }
    else
    {
        // Ethernetからの書き込み要求
        cacheUpdate(_echo, rbuf, EL_DEOJ); // SEOJ=0ef0, DEOJ=0ef0 or 0288
    }
}
