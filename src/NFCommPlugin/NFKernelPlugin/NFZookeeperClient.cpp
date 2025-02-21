﻿// -------------------------------------------------------------------------
//    @FileName         :    NFZookeeperClient.cpp
//    @Author           :    xxxxx
//    @Date             :   xxxx-xx-xx
//    @Email			:    xxxxxxxxx@xxx.xxx
//    @Module           :    NFZookeeperClient.cpp
//
// -------------------------------------------------------------------------

#include "NFZookeeperClient.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFCore/NFTime.h"
#include "NFComm/NFCore/NFSHA1.h"
#include "NFComm/NFCore/NFPebbleSha1.h"
#include "NFComm/NFCore/NFBase64.h"
#include <poll.h>

#define UNRETURN_CODE 999

static void CopyAclVector(ACL_vector *dst_acl, const ACL_vector *src_acl)
{
    allocate_ACL_vector(dst_acl, src_acl->count);
    for (int idx = 0 ; idx < src_acl->count ; ++idx)
    {
        dst_acl->data[idx].perms = src_acl->data[idx].perms;

        int sche_length = strlen(src_acl->data[idx].id.scheme);
        dst_acl->data[idx].id.scheme = reinterpret_cast<char*>(malloc(sche_length + 1));
        memcpy(dst_acl->data[idx].id.scheme, src_acl->data[idx].id.scheme, sche_length + 1);

        int id_length = strlen(src_acl->data[idx].id.id);
        dst_acl->data[idx].id.id = reinterpret_cast<char*>(malloc(id_length + 1));
        memcpy(dst_acl->data[idx].id.id, src_acl->data[idx].id.id, id_length + 1);
    }
}

struct ZkVoidCompletionRsp
{
    ZkVoidCompletionRsp()
            :   _rc(UNRETURN_CODE)
    {
    }

    int _rc;

    inline bool HasRsp() { return (_rc <= 0); }

    void OnRsp(int rc)
    {
        _rc = rc;
    }
};

struct ZkStatCompletionRsp
{
    explicit ZkStatCompletionRsp(Stat* stat)
            :   _rc(UNRETURN_CODE), _stat(stat)
    {
    }

    int _rc;
    Stat* _stat;

    inline bool HasRsp() { return (_rc <= 0); }

    void OnRsp(int rc, const Stat *stat)
    {
        _rc = rc;
        if (0 == rc)
        {
            if (_stat) *_stat = *stat;
        }
    }
};

struct ZkStringCompletionRsp
{
    ZkStringCompletionRsp()
            :   _rc(UNRETURN_CODE)
    {
    }

    int _rc;
    std::string _str;

    inline bool HasRsp() { return (_rc <= 0); }

    void OnRsp(int rc, const char *value)
    {
        _rc = rc;
        if (NULL != value) _str.assign(value);
    }
};

struct ZkStringsCompletionRsp
{
    ZkStringsCompletionRsp(String_vector* strings, Stat *stat)
            :   _rc(UNRETURN_CODE), _strings(strings), _stat(stat)
    {
    }

    int _rc;
    String_vector* _strings;
    Stat* _stat;

    inline bool HasRsp() { return (_rc <= 0); }

    void OnRsp(int rc, const String_vector *strings, const Stat *stat)
    {
        _rc = rc;
        if (0 == rc && NULL != strings)
        {
            allocate_String_vector(_strings, strings->count);
            for (int idx = 0 ; idx < strings->count ; ++idx)
            {
                int length = strlen(strings->data[idx]);
                _strings->data[idx] = reinterpret_cast<char*>(malloc(length + 1));
                memcpy(_strings->data[idx], strings->data[idx], length + 1);
            }
        }
        if (0 == rc && NULL != _stat)
        {
            *_stat = *stat;
        }
    }
};

struct ZkAclCompletionRsp
{
    ZkAclCompletionRsp(ACL_vector *acl, Stat *stat)
            :   _rc(UNRETURN_CODE), _acl(acl), _stat(stat)
    {
    }

    int _rc;
    ACL_vector* _acl;
    Stat* _stat;

    inline bool HasRsp() { return (_rc <= 0); }

    void OnRsp(int rc, ACL_vector *acl, Stat *stat)
    {
        _rc = rc;
        if (0 == rc && NULL != acl)
        {
            CopyAclVector(_acl, acl);
        }
        if (0 == rc && NULL != _stat)
        {
            *_stat = *stat;
        }
    }
};

struct ZkDataCompletionRsp
{
    ZkDataCompletionRsp(char* buffer, int* length, Stat* stat)
            :   _rc(UNRETURN_CODE), _buffer(buffer), _length(length), _stat(stat)
    {
    }

    int _rc;
    char* _buffer;
    int* _length;
    Stat* _stat;

    inline bool HasRsp() { return (_rc <= 0); }

    void OnRsp(int rc, const char *value, int value_len, const Stat *stat)
    {
        _rc = rc;
        if (0 == rc)
        {
            if (_buffer && _length) memcpy(_buffer, value,
                                (((*_length) < value_len) ? (*_length) : value_len));
            if (_length) *_length = value_len;
            if (_stat) *_stat = *stat;
        }
    }
};

template<class ZKCBType>
struct ZkCallBackHolder
{
    explicit ZkCallBackHolder(ZKCBType cb) : _cb(cb) {}

    ZKCBType _cb;
};

struct ZkEphemeralCreateCallBackHolder
{
    explicit ZkEphemeralCreateCallBackHolder(NFZookeeperClient* client, ZkStringCompletionCb cb)
            : _client(client), _cb(cb) {}

    void SetCreateInfo(const char* path, const char* value, int value_length,
                       const ACL_vector* acl)
    {
        _node_info._path.assign(path);
        _node_info._value.assign(value, value_length);
        CopyAclVector(&_node_info._acl_vec, acl);
    }

    NFZookeeperClient* _client;
    EphemeralNodeInfo _node_info;
    ZkStringCompletionCb _cb;
};

void default_void_completion(int rc, const void* data)
{
    ZkCallBackHolder<ZkVoidCompletionCb>* holder =
            const_cast<ZkCallBackHolder<ZkVoidCompletionCb>*>(
                    reinterpret_cast<const ZkCallBackHolder<ZkVoidCompletionCb>*>(data));
    if (NULL != holder)
    {
        if (holder->_cb != NULL)
        {
            holder->_cb(rc);
        }
        delete holder;
    }
}

void default_stat_completion(int rc, const struct Stat *stat, const void* data)
{
    ZkCallBackHolder<ZkStatCompletionCb>* holder =
            const_cast<ZkCallBackHolder<ZkStatCompletionCb>*>(
                    reinterpret_cast<const ZkCallBackHolder<ZkStatCompletionCb>*>(data));
    if (NULL != holder)
    {
        if (holder->_cb != NULL)
        {
            holder->_cb(rc, stat);
        }
        delete holder;
    }
}

void default_string_completion(int rc, const char *value, const void* data)
{
    ZkCallBackHolder<ZkStringCompletionCb>* holder =
            const_cast<ZkCallBackHolder<ZkStringCompletionCb>*>(
                    reinterpret_cast<const ZkCallBackHolder<ZkStringCompletionCb>*>(data));
    if (NULL != holder)
    {
        if (holder->_cb != NULL)
        {
            holder->_cb(rc, value);
        }
        delete holder;
    }
}

void default_strings_stat_completion(int rc, const String_vector *strings,
                                     const Stat *stat, const void *data)
{
    ZkCallBackHolder<ZkStringsCompletionCb>* holder =
            const_cast<ZkCallBackHolder<ZkStringsCompletionCb>*>(
                    reinterpret_cast<const ZkCallBackHolder<ZkStringsCompletionCb>*>(data));
    if (NULL != holder)
    {
        if (holder->_cb != NULL)
        {
            holder->_cb(rc, strings, stat);
        }
        delete holder;
    }
}

void default_acl_completion(int rc, ACL_vector *acl, Stat *stat, const void *data)
{
    ZkCallBackHolder<ZkAclCompletionCb>* holder =
            const_cast<ZkCallBackHolder<ZkAclCompletionCb>*>(
                    reinterpret_cast<const ZkCallBackHolder<ZkAclCompletionCb>*>(data));
    if (NULL != holder)
    {
        if (holder->_cb != NULL)
        {
            holder->_cb(rc, acl, stat);
        }
        delete holder;
    }
}

void default_data_completion(int rc, const char *value, int value_len,
                             const struct Stat *stat, const void* data)
{
    ZkCallBackHolder<ZkDataCompletionCb>* holder =
            const_cast<ZkCallBackHolder<ZkDataCompletionCb>*>(
                    reinterpret_cast<const ZkCallBackHolder<ZkDataCompletionCb>*>(data));
    if (NULL != holder)
    {
        if (holder->_cb != NULL)
        {
            holder->_cb(rc, value, value_len, stat);
        }
        delete holder;
    }
}

void ConnectWatcher(zhandle_t* zh, int type, int state, const char *path, void *watcherCtx)
{
    NFZookeeperClient* zkclient = reinterpret_cast<NFZookeeperClient*>(watcherCtx);

    if (ZOO_SESSION_EVENT == type)
    {
        switch (state)
        {
            case -112: /*ZOO_EXPIRED_SESSION_STATE*/
            case -113: /*ZOO_AUTH_FAILED_STATE*/
                NFLogError(NF_LOG_SYSTEMLOG, 0, "session event: {}", state == -112 ? "session expired" : "auth failed");
                zkclient->AConnect();
                zkclient->SetConnectionState(state);
                break;
            case 3: /*ZOO_CONNECTED_STATE*/
                zkclient->OnConnected();
                zkclient->SetConnectionState(state);
                break;
            default:
                break;
        }
        return;
    }

    std::function<void(int, const char*)> cb = zkclient->GetWatchCallback();
    if (cb)
    {
        cb(type, path);
    }
}

EphemeralNodeInfo::EphemeralNodeInfo()
{
    _acl_vec.count = 0;
    _acl_vec.data = NULL;
    _state = kNODE_INIT;
}

EphemeralNodeInfo::EphemeralNodeInfo(const EphemeralNodeInfo& rhs)
{
    _path = rhs._path;
    _value = rhs._value;
    _state = rhs._state;
    CopyAclVector(&_acl_vec, &rhs._acl_vec);
}

EphemeralNodeInfo::~EphemeralNodeInfo()
{
    deallocate_ACL_vector(&_acl_vec);
}

EphemeralNodeInfo& EphemeralNodeInfo::operator = (const EphemeralNodeInfo& rhs)
{
    _path = rhs._path;
    _value = rhs._value;
    _state = rhs._state;
    CopyAclVector(&_acl_vec, &rhs._acl_vec);
    return *this;
}

NFZookeeperClient::NFZookeeperClient() {
    m_time_out_ms  = 30000;
    m_last_update_time = 0;
    m_last_resume_time = 0;
    m_zk_path = "/";
    m_zk_handle = NULL;
    m_watch_cb = NULL;
    m_state = 0;
    logLevel = ZOO_LOG_LEVEL_ERROR;
}

NFZookeeperClient::~NFZookeeperClient() {
    Close();
}

int NFZookeeperClient::Init(const string &host, int time_out, std::string zk_path) {
    m_zk_host = host;
    m_time_out_ms = time_out;
    m_zk_path = zk_path;
    m_auths_set.clear();
    m_get_watch.clear();
    m_get_child_watch.clear();
    m_exist_watch.clear();
    return 0;
}

int NFZookeeperClient::AConnect() {
    if (NULL != m_zk_handle)
    {
        // 连接断开或鉴权失败才需要重新连接，其它状态连接都有效或正在连接中
        int state = zoo_state(m_zk_handle);
        if (ZOO_EXPIRED_SESSION_STATE != state && ZOO_AUTH_FAILED_STATE != state)
        {
            return 0;
        }

        NFLogTrace(NF_LOG_SYSTEMLOG, 0, "zk reinit, sessionid={}", zoo_client_id(m_zk_handle)->client_id);
        Close();
    }

    // 过期之后初始化zkapi不能使用老的sessionid，会导致循环过期(即重连后server又返回过期，又继续...)
    m_zk_handle = zookeeper_init(m_zk_host.c_str(), ConnectWatcher,
                                 m_time_out_ms, NULL, this, 0);
    if (NULL == m_zk_handle)
    {
        return ZNOTHING;
    }
    return 0;
}

void NFZookeeperClient::OnConnected() {
    // 恢复鉴权
    std::set<std::string> old_auths(m_auths_set);
    m_auths_set.clear();
    for (std::set<std::string>::iterator it = old_auths.begin();
         it != old_auths.end(); ++it)
    {
        AddDigestAuth(*it);
    }
    // 恢复watch
    for (std::set<std::string>::iterator it = m_get_watch.begin() ;
         it != m_get_watch.end() ; ++it)
    {
        AGet(it->c_str(), 1, NULL);
    }
    for (std::set<std::string>::iterator it = m_get_child_watch.begin() ;
         it != m_get_child_watch.end() ; ++it)
    {
        AGetChildren(it->c_str(), 1, NULL);
    }
    for (std::set<std::string>::iterator it = m_exist_watch.begin() ;
         it != m_exist_watch.end() ; ++it)
    {
        AExists(it->c_str(), 1, NULL);
    }
    // 恢复临时节点
    m_last_resume_time = NFTime::Now().UnixMSec();
    std::set<EphemeralNodeInfo> resume_nodes;
    for (std::set<EphemeralNodeInfo>::iterator it = m_ephemeral_node.begin() ;
         it != m_ephemeral_node.end() ; ++it)
    {
        EphemeralNodeInfo node_info(*it);

        // 已经有恢复标记的，不再恢复，统一放到周期恢复中去重试
        if (it->_state != kNODE_RESUME) {
            if (m_state == -112 || m_state == -113) {
                node_info._state = kNODE_RESUME;
            }
            ACreate(it->_path.c_str(), it->_value.c_str(), it->_value.length(),
                    &(it->_acl_vec), ZOO_EPHEMERAL, NULL);
        }

        resume_nodes.insert(node_info);
    }
    m_ephemeral_node.swap(resume_nodes);
}

int NFZookeeperClient::Connect() {
    int max_retry = 10;
    int ret = AConnect();
    while ((--max_retry) && 0 <= ret)
    {
        Update(true);
        ret = zoo_state(m_zk_handle);
        if (ZOO_CONNECTED_STATE == ret ||
            ZOO_AUTH_FAILED_STATE == ret ||
            ZOO_EXPIRED_SESSION_STATE == ret)
        {
            break;
        }
    }

    if (ZOO_CONNECTED_STATE == ret)
    {
        return 0;
    }
    // 特殊的ZK_CLOSED_STATE，值为0
    return ((ret == 0) ? proto_ff::ERR_CODE_ZK_CONNECTIONLOSS : ret);
}

int NFZookeeperClient::ACreate(const char* path, const char* value, int value_length,
                             const ACL_vector* acl, int flags, ZkStringCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    int ret = 0;
    if (flags == ZOO_EPHEMERAL)
    {
        ZkEphemeralCreateCallBackHolder *holder =
                new ZkEphemeralCreateCallBackHolder(this, cb);
        holder->SetCreateInfo(path, value, value_length, acl);
        ret = zoo_acreate(m_zk_handle, path, value, value_length,
                          acl, flags, &(NFZookeeperClient::EphemeralNodeCreateCallback), holder);
        if (0 != ret)
        {
            NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] acreate ephemeral node failed ret[{}]", path, ret);
            delete holder;
        }
    }
    else
    {
        ZkCallBackHolder<ZkStringCompletionCb> *holder =
                new ZkCallBackHolder<ZkStringCompletionCb>(cb);
        ret = zoo_acreate(m_zk_handle, path, value, value_length,
                          acl, flags, default_string_completion, holder);
        if (0 != ret)
        {
            NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] acreate failed ret[{}]", path, ret);
            delete holder;
        }
    }
    return ret;
}


int NFZookeeperClient::Create(const char* path, const char* value, int value_length,
                            const ACL_vector* acl, int flags)
{
    ZkStringCompletionRsp rsp;
    ZkStringCompletionCb cb = std::bind(&ZkStringCompletionRsp::OnRsp,
                                        &rsp, std::placeholders::_1, std::placeholders::_2);
    int ret = ACreate(path, value, value_length, acl, flags, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}

int NFZookeeperClient::AGet(const char* path, int watch, ZkDataCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    if (0 != watch && 0 == m_get_watch.count(path))
    {
        m_get_watch.insert(path);
    }

    ZkCallBackHolder<ZkDataCompletionCb>* holder =
            new ZkCallBackHolder<ZkDataCompletionCb>(cb);
    int ret = zoo_aget(m_zk_handle, path, watch, default_data_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] aget failed ret[{}]", path, ret);
        delete holder;
    }
    return ret;
}


int NFZookeeperClient::Get(const char* path, int watch, char* buffer, int* length, Stat* stat)
{
    ZkDataCompletionRsp rsp(buffer, length, stat);
    ZkDataCompletionCb cb = std::bind(&ZkDataCompletionRsp::OnRsp,
                                      &rsp, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    int ret = AGet(path, watch, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}


int NFZookeeperClient::ASet(const char* path, const char* buffer, int buflen, int version,
                          ZkStatCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    ZkCallBackHolder<ZkStatCompletionCb>* holder =
            new ZkCallBackHolder<ZkStatCompletionCb>(cb);
    int ret = zoo_aset(m_zk_handle, path, buffer, buflen, version,
                       default_stat_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] aset failed ret[{}]", path, ret);
        delete holder;
    }
    return ret;
}

int NFZookeeperClient::Set(const char* path, const char* buffer, int buflen, int version, Stat* stat)
{
    ZkStatCompletionRsp rsp(stat);
    ZkStatCompletionCb cb = std::bind(&ZkStatCompletionRsp::OnRsp,
                                      &rsp, std::placeholders::_1, std::placeholders::_2);
    int ret = ASet(path, buffer, buflen, version, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}


int NFZookeeperClient::ADelete(const char* path, int version, ZkVoidCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    ZkCallBackHolder<ZkVoidCompletionCb>* holder =
            new ZkCallBackHolder<ZkVoidCompletionCb>(cb);
    int ret = zoo_adelete(m_zk_handle, path, version, default_void_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] adelete failed ret[{}]", path, ret);
        delete holder;
    }
    // 删除临时节点记录信息
    EphemeralNodeInfo node_info;
    node_info._path.assign(path);
    m_ephemeral_node.erase(node_info);
    return ret;
}

int NFZookeeperClient::Delete(const char* path, int version)
{
    ZkVoidCompletionRsp rsp;
    ZkVoidCompletionCb cb = std::bind(&ZkVoidCompletionRsp::OnRsp, &rsp, std::placeholders::_1);
    int ret = ADelete(path, version, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}

int NFZookeeperClient::AExists(const char* path, int watch, ZkStatCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    if (0 != watch && 0 == m_exist_watch.count(path))
    {
        m_exist_watch.insert(path);
    }

    ZkCallBackHolder<ZkStatCompletionCb>* holder =
            new ZkCallBackHolder<ZkStatCompletionCb>(cb);
    int ret = zoo_aexists(m_zk_handle, path, watch, default_stat_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[%s] aexists failed ret[%d]", path, ret);
        delete holder;
    }
    return ret;
}

int NFZookeeperClient::Exists(const char* path, int watch, Stat* stats)
{
    ZkStatCompletionRsp rsp(stats);
    ZkStatCompletionCb cb = std::bind(&ZkStatCompletionRsp::OnRsp,
                                      &rsp, std::placeholders::_1, std::placeholders::_2);
    int ret = AExists(path, watch, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}

int NFZookeeperClient::AWExists(const char* path, watcher_fn watcher, void* watcherCtx,
                              ZkStatCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    ZkCallBackHolder<ZkStatCompletionCb>* holder =
            new ZkCallBackHolder<ZkStatCompletionCb>(cb);
    int ret = zoo_awexists(m_zk_handle, path, watcher, watcherCtx,
                           default_stat_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] awexists failed ret[{}]", path, ret);
        delete holder;
    }
    return ret;
}

int NFZookeeperClient::WExists(const char* path, watcher_fn watcher, void* watcherCtx, Stat* stat)
{
    ZkStatCompletionRsp rsp(stat);
    ZkStatCompletionCb cb = std::bind(&ZkStatCompletionRsp::OnRsp,
                                      &rsp, std::placeholders::_1, std::placeholders::_2);
    int ret = AWExists(path, watcher, watcherCtx, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}

int NFZookeeperClient::AGetChildren(const char* path, int watch, ZkStringsCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    if (0 != watch && 0 == m_get_child_watch.count(path))
    {
        m_get_child_watch.insert(path);
    }

    ZkCallBackHolder<ZkStringsCompletionCb>* holder =
            new ZkCallBackHolder<ZkStringsCompletionCb>(cb);
    int ret = zoo_aget_children2(m_zk_handle, path, watch,
                                 default_strings_stat_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] aget_children failed ret[{}]", path, ret);
        delete holder;
    }
    return ret;
}

int NFZookeeperClient::GetChildren(const char* path, int watch, String_vector* children, Stat* stat)
{
    ZkStringsCompletionRsp rsp(children, stat);
    ZkStringsCompletionCb cb = std::bind(&ZkStringsCompletionRsp::OnRsp,
                                         &rsp, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    int ret = AGetChildren(path, watch, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}
int NFZookeeperClient::AGetAcl(const char *path, ZkAclCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    ZkCallBackHolder<ZkAclCompletionCb>* holder =
            new ZkCallBackHolder<ZkAclCompletionCb>(cb);
    int ret = zoo_aget_acl(m_zk_handle, path, default_acl_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] aget_acl failed ret[{}]", path, ret);
        delete holder;
    }
    return ret;
}

int NFZookeeperClient::GetAcl(const char *path, struct ACL_vector* acl)
{
    ZkAclCompletionRsp rsp(acl, NULL);
    ZkAclCompletionCb cb = std::bind(&ZkAclCompletionRsp::OnRsp,
                                     &rsp, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    int ret = AGetAcl(path, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}
int NFZookeeperClient::ASetAcl(const char *path, int version, ACL_vector* acl,
                             ZkVoidCompletionCb cb)
{
    if (NULL == m_zk_handle || NULL == path)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    ZkCallBackHolder<ZkVoidCompletionCb>* holder =
            new ZkCallBackHolder<ZkVoidCompletionCb>(cb);
    int ret = zoo_aset_acl(m_zk_handle, path, version, acl, default_void_completion, holder);
    if (0 != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, 0, "path[{}] aset_acl failed ret[{}]", path, ret);
        delete holder;
    }
    return ret;
}

int NFZookeeperClient::SetAcl(const char *path, int version, ACL_vector* acl)
{
    ZkVoidCompletionRsp rsp;
    ZkVoidCompletionCb cb = std::bind(&ZkVoidCompletionRsp::OnRsp, &rsp, std::placeholders::_1);
    int ret = ASetAcl(path, version, acl, cb);
    if (0 != ret)
    {
        return ret;
    }

    // wait for callback
    while (!rsp.HasRsp())
    {
        Update(true);
    }
    return rsp._rc;
}

int NFZookeeperClient::Update(bool is_block)
{
    // 未连接成功时，异步update频率控制为1s/次
    int state = zoo_state(m_zk_handle);
    if (false == is_block && ZOO_CONNECTED_STATE != state)
    {
        int now = static_cast<int>(time(NULL));
        if (now <= m_last_update_time)
        {
            return -1;
        }
        m_last_update_time = now;
    }

    pollfd pfd = { -1 };
    int interest = 0;
    timeval timeout = { 0, 0 };
    zookeeper_interest(m_zk_handle, &pfd.fd, &interest, &timeout);
    if (pfd.fd < 0)
        return -1;

    int wait_time = (is_block ? (timeout.tv_sec * 1000 + timeout.tv_usec / 1000) : 0);
    pfd.events = ((interest & ZOOKEEPER_WRITE) ? POLLOUT : 0);
    pfd.events |= ((interest & ZOOKEEPER_READ) ? POLLIN : 0);
    poll(&pfd, 1, wait_time);

    interest = ((pfd.revents & POLLIN) ? ZOOKEEPER_READ : 0);
    interest |= ((pfd.revents & POLLOUT) ? ZOOKEEPER_WRITE : 0);
    interest |= ((pfd.revents & POLLHUP) ? ZOOKEEPER_WRITE : 0);
    zookeeper_process(m_zk_handle, interest);

    ResumeEphemeralNode();

    return (interest == 0 ? -1 : 0);
}

int NFZookeeperClient::Close(bool is_clean)
{
    if (m_zk_handle != NULL)
    {
        zookeeper_close(m_zk_handle);
    }
    m_zk_handle = NULL;

    if (true == is_clean)
    {
        m_auths_set.clear();
        m_get_watch.clear();
        m_get_child_watch.clear();
        m_exist_watch.clear();
        m_ephemeral_node.clear();
    }
    return 0;
}

int NFZookeeperClient::AddDigestAuth(const std::string& digest_auth)
{
    if (NULL == m_zk_handle)
    {
        return proto_ff::ERR_CODE_ZK_APIERROR;
    }

    std::set<std::string>::iterator iter;
    if (m_auths_set.end() != m_auths_set.find(digest_auth))
    {
        return 0;
    }
    m_auths_set.insert(digest_auth);
    int ret = zoo_add_auth(m_zk_handle, "digest", digest_auth.c_str(),
                           digest_auth.length(), NULL, NULL);
    return ret;
}

std::string NFZookeeperClient::DigestEncrypt(const std::string& id_passwd)
{
    std::string encrypted_pwd;
    std::string::size_type pos = id_passwd.find(":");
    if (std::string::npos == pos)
    {
        return "";
    }

    NFSHA1 temp;
    std::string temp_buffer;
    temp_buffer = temp.sha1str(id_passwd.data(), id_passwd.length());

    NFPebbleSha1 sha1;
    char buffer[1024];
    sha1.Encode2Ascii(id_passwd.data(), buffer);

    std::string src;
    src.assign(buffer, 20);

    encrypted_pwd = NFBase64::Encode(src);

    encrypted_pwd = id_passwd.substr(0, pos + 1) + encrypted_pwd;

    return encrypted_pwd;
}

void NFZookeeperClient::EphemeralNodeCreateCallback(int rc, const char *value, const void* data)
{
    ZkEphemeralCreateCallBackHolder* holder = const_cast<ZkEphemeralCreateCallBackHolder*>(
            reinterpret_cast<const ZkEphemeralCreateCallBackHolder*>(data));
    if (NULL != holder)
    {
        // 将创建成功了的临时节点记录下来
        if (rc == 0)
        {
            // 创建成功重置恢复标记
            holder->_node_info._state = kNODE_INIT;
            // 不管是首次创建还是恢复先删除原节点(考虑到上面有清理标记操作，简单起见逻辑上不再细分)
            holder->_client->m_ephemeral_node.erase(holder->_node_info);
            holder->_client->m_ephemeral_node.insert(holder->_node_info);
            NFLogTrace(NF_LOG_SYSTEMLOG, 0, "create {} success", holder->_node_info._path.c_str());
        } else {
            // 创建临时节点失败(若为恢复失败，恢复标记不变，等下周期重新恢复)
            if (rc != proto_ff::ERR_CODE_ZK_NODEEXISTS)
            {
                NFLogError(NF_LOG_SYSTEMLOG, 0, "create {} failed({})", holder->_node_info._path.c_str(), rc);
            }
        }
        if (holder->_cb != NULL)
        {
            holder->_cb(rc, value);
        }
        delete holder;
    }
}

void NFZookeeperClient::ResumeEphemeralNode()
{
    if (m_last_resume_time == 0) {
        return;
    }

    int64_t now = NFTime::Now().UnixMSec();
    if (m_last_resume_time + m_time_out_ms > now) {
        // 恢复重试周期和会话超时时间保持一致
        return;
    }

    int resume_num = 0;
    for (std::set<EphemeralNodeInfo>::iterator it = m_ephemeral_node.begin() ;
         it != m_ephemeral_node.end() ; ++it)
    {
        // 对恢复失败的进行重新恢复
        if (it->_state == kNODE_RESUME) {
            ACreate(it->_path.c_str(), it->_value.c_str(), it->_value.length(),
                    &(it->_acl_vec), ZOO_EPHEMERAL, NULL);
            resume_num++;
        }
    }

    if (resume_num > 0) {
        m_last_resume_time = now;
    } else {
        m_last_resume_time = 0;
    }
    //NFLogTrace(NF_LOG_SYSTEMLOG, 0, "resume {} nodes", resume_num);
}
