// Thin SDK ABI adapter. Login policy, roster admission, framing and retries
// live in Rust. Every struct version/layout comes from the official headers.
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include "src/eos_sdk.hpp"
#include <deque>
#include <mutex>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <regex>
#include <algorithm>
#ifdef HALO2_EOS_SDK
#include <eos_sdk.h>
#include <eos_connect.h>
#include <eos_p2p.h>
#include <eos_logging.h>
#include <eos_sessions.h>
#include <chrono>
#include <thread>
#endif

namespace halo2::online_protocol_bridge {
#ifdef HALO2_EOS_SDK
using enum EOS_EResult;
using enum EOS_EExternalCredentialType;
using enum EOS_ERelayControl;
using enum EOS_EPacketReliability;
namespace {
std::recursive_mutex sdk_mutex;
unsigned sdk_owners{};
void check(EOS_EResult result) {
  if(result!=EOS_Success)throw std::runtime_error(std::string("EOS SDK: ")+EOS_EResult_ToString(result));
}
std::string user_text(EOS_ProductUserId id) {
  char text[EOS_PRODUCTUSERID_MAX_LENGTH+1]{};int32_t size=sizeof(text);
  check(EOS_ProductUserId_ToString(id,text,&size));return text;
}
int32_t outcome(EOS_EResult code) {
  if(code==EOS_Success)return 0;
  if(code==EOS_DuplicateNotAllowed)return 1;
  if(code==EOS_InvalidUser)return 2;
  return 100+static_cast<int32_t>(code);
}
}
struct RawEos::Impl {
  EOS_HPlatform platform{};
  EOS_HConnect connect{};
  EOS_HP2P p2p{};
  EOS_HSessions sessions{};
  EOS_ProductUserId local{};
  EOS_ContinuanceToken continuation{};
  EOS_P2P_SocketId socket{};
  EOS_NotificationId request{},established{},closed{},expired{};
  EOS_NotificationId invitations{};
  std::string session_name;
  bool session_active{},session_destroyed{};
  bool invite_query_pending{};
  std::deque<EosEvent> events;
  bool owns{};
  explicit Impl(const EosConfig &config) {
    std::lock_guard lock(sdk_mutex);
    if(!sdk_owners){EOS_InitializeOptions options{};options.ApiVersion=EOS_INITIALIZE_API_LATEST;
      options.ProductName="H2Forever";options.ProductVersion="1";check(EOS_Initialize(&options));
      if(std::getenv("H2_EOS_DIAGNOSTICS")){
        EOS_Logging_SetCallback([](const EOS_LogMessage *message){
          if(!message||!message->Category||!message->Message)return;
          const std::string category(message->Category);
          if(category.find("P2P")==std::string::npos&&category.find("RTC")==std::string::npos&&category.find("Sessions")==std::string::npos&&message->Level>EOS_ELogLevel::EOS_LOG_Warning)return;
          try{const auto safe=std::regex_replace(std::string(message->Message),std::regex("[A-Za-z0-9_+/=-]{24,}"),"[redacted]");
            std::fprintf(stderr,"EOS %s: %s\n",message->Category,safe.c_str());}catch(...){}
        });
        EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_P2P,EOS_ELogLevel::EOS_LOG_VeryVerbose);
        EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_RTC,EOS_ELogLevel::EOS_LOG_VeryVerbose);
        EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_Sessions,EOS_ELogLevel::EOS_LOG_VeryVerbose);
      }
    }
    ++sdk_owners;owns=true;
    try {
      auto strings=config; // rust::String::c_str() may add its terminator.
      EOS_Platform_Options options{};options.ApiVersion=EOS_PLATFORM_OPTIONS_API_LATEST;
      options.ProductId=strings.product.c_str();options.SandboxId=strings.sandbox.c_str();
      options.DeploymentId=strings.deployment.c_str();
      options.ClientCredentials.ClientId=strings.client.c_str();options.ClientCredentials.ClientSecret=strings.secret.c_str();
      options.CacheDirectory=strings.cache.c_str();options.TickBudgetInMilliseconds=2;
      options.Flags=EOS_PF_DISABLE_OVERLAY|EOS_PF_DISABLE_SOCIAL_OVERLAY;
      platform=EOS_Platform_Create(&options);
      if(!platform)throw std::runtime_error("Unable to create the EOS platform.");
      connect=EOS_Platform_GetConnectInterface(platform);p2p=EOS_Platform_GetP2PInterface(platform);
      sessions=EOS_Platform_GetSessionsInterface(platform);
      if(!connect||!p2p)throw std::runtime_error("EOS interfaces are unavailable.");
    }catch(...){release();throw;}
  }
  void release()noexcept {
    if(platform){
      if(request)EOS_P2P_RemoveNotifyPeerConnectionRequest(p2p,request);
      if(established)EOS_P2P_RemoveNotifyPeerConnectionEstablished(p2p,established);
      if(closed)EOS_P2P_RemoveNotifyPeerConnectionClosed(p2p,closed);
      if(expired)EOS_Connect_RemoveNotifyAuthExpiration(connect,expired);
      if(invitations)EOS_Sessions_RemoveNotifySessionInviteReceived(sessions,invitations);
      if(session_active){
        EOS_Sessions_DestroySessionOptions options{};options.ApiVersion=EOS_SESSIONS_DESTROYSESSION_API_LATEST;options.SessionName=session_name.c_str();
        EOS_Sessions_DestroySession(sessions,&options,this,[](const EOS_Sessions_DestroySessionCallbackInfo *data){
          static_cast<Impl*>(data->ClientData)->session_destroyed=true;});
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(!session_destroyed&&std::chrono::steady_clock::now()<deadline){EOS_Platform_Tick(platform);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
      }
      if(local&&request){EOS_P2P_CloseConnectionsOptions options{};options.ApiVersion=EOS_P2P_CLOSECONNECTIONS_API_LATEST;
        options.LocalUserId=local;options.SocketId=&socket;EOS_P2P_CloseConnections(p2p,&options);}
      // Platform release cancels pending callbacks before their ClientData dies.
      EOS_Platform_Release(platform);platform=nullptr;
    }
    if(owns){owns=false;if(!--sdk_owners)EOS_Shutdown();}
  }
  ~Impl(){std::lock_guard lock(sdk_mutex);release();}
  void event(std::uint8_t kind,int32_t code,EOS_ProductUserId user=nullptr,const EOS_P2P_SocketId *id=nullptr) {
    if(events.size()>=512)return;
    EosEvent event{};event.kind=kind;event.code=code;
    if(user)event.user=user_text(user);
    if(id)event.socket=std::string(id->SocketName,strnlen(id->SocketName,sizeof(id->SocketName)));
    events.push_back(std::move(event));
  }
  void invitation(const char *invite,EOS_ProductUserId sender=nullptr) {
    if(!invite||events.size()>=512)return;
    EosEvent event{};event.kind=9;event.socket=invite;
    if(sender)event.user=user_text(sender);
    events.push_back(std::move(event));
  }
};
RawEos::RawEos(const EosConfig &config):impl_(std::make_unique<Impl>(config)){}
RawEos::~RawEos()=default;
void RawEos::sdk_device(){std::lock_guard lock(sdk_mutex);
  EOS_Connect_CreateDeviceIdOptions options{};options.ApiVersion=EOS_CONNECT_CREATEDEVICEID_API_LATEST;options.DeviceModel="H2Forever PC";
  EOS_Connect_CreateDeviceId(impl_->connect,&options,impl_.get(),[](const EOS_Connect_CreateDeviceIdCallbackInfo *data){
    static_cast<Impl*>(data->ClientData)->event(1,outcome(data->ResultCode));});
}
void RawEos::sdk_login(){std::lock_guard lock(sdk_mutex);
  EOS_Connect_Credentials credentials{};credentials.ApiVersion=EOS_CONNECT_CREDENTIALS_API_LATEST;credentials.Type=EOS_ECT_DEVICEID_ACCESS_TOKEN;
  EOS_Connect_UserLoginInfo user{};user.ApiVersion=EOS_CONNECT_USERLOGININFO_API_LATEST;user.DisplayName="H2Forever Player";
  EOS_Connect_LoginOptions options{};options.ApiVersion=EOS_CONNECT_LOGIN_API_LATEST;options.Credentials=&credentials;options.UserLoginInfo=&user;
  EOS_Connect_Login(impl_->connect,&options,impl_.get(),[](const EOS_Connect_LoginCallbackInfo *data){
    auto &self=*static_cast<Impl*>(data->ClientData);self.continuation=data->ContinuanceToken;
    if(data->ResultCode==EOS_Success)self.local=data->LocalUserId;
    self.event(2,outcome(data->ResultCode),data->ResultCode==EOS_Success?data->LocalUserId:nullptr);});
}
void RawEos::sdk_create_user(){std::lock_guard lock(sdk_mutex);
  if(!impl_->continuation)throw std::runtime_error("EOS login did not supply a continuance token.");
  EOS_Connect_CreateUserOptions options{};options.ApiVersion=EOS_CONNECT_CREATEUSER_API_LATEST;options.ContinuanceToken=impl_->continuation;
  EOS_Connect_CreateUser(impl_->connect,&options,impl_.get(),[](const EOS_Connect_CreateUserCallbackInfo *data){
    auto &self=*static_cast<Impl*>(data->ClientData);if(data->ResultCode==EOS_Success)self.local=data->LocalUserId;
    self.event(3,outcome(data->ResultCode),data->ResultCode==EOS_Success?data->LocalUserId:nullptr);});
}
void RawEos::sdk_external_login(rust::Str token){std::lock_guard lock(sdk_mutex);
  const std::string text(token);EOS_Connect_Credentials credentials{};credentials.ApiVersion=EOS_CONNECT_CREDENTIALS_API_LATEST;
  credentials.Type=EOS_ECT_OPENID_ACCESS_TOKEN;credentials.Token=text.c_str();
  EOS_Connect_LoginOptions options{};options.ApiVersion=EOS_CONNECT_LOGIN_API_LATEST;options.Credentials=&credentials;
  EOS_Connect_Login(impl_->connect,&options,impl_.get(),[](const EOS_Connect_LoginCallbackInfo *data){
    if(!EOS_EResult_IsOperationComplete(data->ResultCode))return;
    auto &self=*static_cast<Impl*>(data->ClientData);self.continuation=data->ContinuanceToken;
    if(data->ResultCode==EOS_Success)self.local=data->LocalUserId;
    self.event(2,outcome(data->ResultCode),data->ResultCode==EOS_Success?data->LocalUserId:nullptr);
  });
}
rust::String RawEos::sdk_id_token()const{std::lock_guard lock(sdk_mutex);
  EOS_Connect_CopyIdTokenOptions options{};options.ApiVersion=EOS_CONNECT_COPYIDTOKEN_API_LATEST;options.LocalUserId=impl_->local;
  EOS_Connect_IdToken *value{};check(EOS_Connect_CopyIdToken(impl_->connect,&options,&value));
  std::unique_ptr<EOS_Connect_IdToken,decltype(&EOS_Connect_IdToken_Release)> owned(value,EOS_Connect_IdToken_Release);
  if(!value->JsonWebToken)throw std::runtime_error("EOS identity token unavailable");
  return value->JsonWebToken;
}
void RawEos::sdk_socket(rust::Str name,bool force){std::lock_guard lock(sdk_mutex);
  if(!impl_->local||name.empty()||name.size()>32||impl_->request)throw std::runtime_error("Invalid EOS socket lifecycle.");
  impl_->socket.ApiVersion=EOS_P2P_SOCKETID_API_LATEST;std::memcpy(impl_->socket.SocketName,name.data(),name.size());
  EOS_P2P_SetRelayControlOptions relay{};relay.ApiVersion=EOS_P2P_SETRELAYCONTROL_API_LATEST;
  relay.RelayControl=force?EOS_RC_ForceRelays:EOS_RC_AllowRelays;check(EOS_P2P_SetRelayControl(impl_->p2p,&relay));
  EOS_P2P_SetPacketQueueSizeOptions queues{};queues.ApiVersion=EOS_P2P_SETPACKETQUEUESIZE_API_LATEST;
  queues.IncomingPacketQueueMaxSizeBytes=512*1170;queues.OutgoingPacketQueueMaxSizeBytes=65536;
  check(EOS_P2P_SetPacketQueueSize(impl_->p2p,&queues));
  EOS_P2P_AddNotifyPeerConnectionRequestOptions request{};request.ApiVersion=EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
  request.LocalUserId=impl_->local;request.SocketId=&impl_->socket;
  impl_->request=EOS_P2P_AddNotifyPeerConnectionRequest(impl_->p2p,&request,impl_.get(),[](const EOS_P2P_OnIncomingConnectionRequestInfo *data){
    static_cast<Impl*>(data->ClientData)->event(4,0,data->RemoteUserId,data->SocketId);});
  EOS_P2P_AddNotifyPeerConnectionEstablishedOptions established{};established.ApiVersion=EOS_P2P_ADDNOTIFYPEERCONNECTIONESTABLISHED_API_LATEST;
  established.LocalUserId=impl_->local;established.SocketId=&impl_->socket;
  impl_->established=EOS_P2P_AddNotifyPeerConnectionEstablished(impl_->p2p,&established,impl_.get(),[](const EOS_P2P_OnPeerConnectionEstablishedInfo *data){
    static_cast<Impl*>(data->ClientData)->event(5,static_cast<int32_t>(data->NetworkType),data->RemoteUserId,data->SocketId);});
  EOS_P2P_AddNotifyPeerConnectionClosedOptions closed{};closed.ApiVersion=EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
  closed.LocalUserId=impl_->local;closed.SocketId=&impl_->socket;
  impl_->closed=EOS_P2P_AddNotifyPeerConnectionClosed(impl_->p2p,&closed,impl_.get(),[](const EOS_P2P_OnRemoteConnectionClosedInfo *data){
    static_cast<Impl*>(data->ClientData)->event(6,static_cast<int32_t>(data->Reason),data->RemoteUserId,data->SocketId);});
  EOS_Connect_AddNotifyAuthExpirationOptions expiration{};expiration.ApiVersion=EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;
  impl_->expired=EOS_Connect_AddNotifyAuthExpiration(impl_->connect,&expiration,impl_.get(),[](const EOS_Connect_AuthExpirationCallbackInfo *data){
    static_cast<Impl*>(data->ClientData)->event(7,0);});
  impl_->session_name="H2"+std::string(name);
  EOS_Sessions_AddNotifySessionInviteReceivedOptions invitations{};invitations.ApiVersion=EOS_SESSIONS_ADDNOTIFYSESSIONINVITERECEIVED_API_LATEST;
  impl_->invitations=EOS_Sessions_AddNotifySessionInviteReceived(impl_->sessions,&invitations,impl_.get(),[](const EOS_Sessions_SessionInviteReceivedCallbackInfo *data){
    auto &self=*static_cast<Impl*>(data->ClientData);
    self.invitation(data->InviteId,data->TargetUserId);
  });
  if(!impl_->request||!impl_->established||!impl_->closed||!impl_->expired||!impl_->invitations)throw std::runtime_error("EOS notification registration failed.");
}
rust::Vec<EosEvent> RawEos::sdk_poll(){std::lock_guard lock(sdk_mutex);
  EOS_Platform_Tick(impl_->platform);
  rust::Vec<EosEvent> result;
  while(!impl_->events.empty()){result.push_back(std::move(impl_->events.front()));impl_->events.pop_front();}
  if(!impl_->local||!impl_->request)return result;
  for(unsigned i=0;i<512;++i){
    EOS_P2P_ReceivePacketOptions options{};options.ApiVersion=EOS_P2P_RECEIVEPACKET_API_LATEST;
    options.LocalUserId=impl_->local;options.MaxDataSizeBytes=EOS_P2P_MAX_PACKET_SIZE;
    EOS_ProductUserId peer{};EOS_P2P_SocketId socket{};std::uint8_t channel{},bytes[EOS_P2P_MAX_PACKET_SIZE];std::uint32_t size{};
    const auto status=EOS_P2P_ReceivePacket(impl_->p2p,&options,&peer,&socket,&channel,bytes,&size);
    if(status==EOS_NotFound)break;
    check(status);if(channel||size>sizeof(bytes))continue;
    EosEvent event{};event.kind=8;event.user=user_text(peer);event.socket=std::string(socket.SocketName,strnlen(socket.SocketName,sizeof(socket.SocketName)));
    event.data.reserve(size);for(unsigned j=0;j<size;++j)event.data.push_back(bytes[j]);result.push_back(std::move(event));
  }
  return result;
}
bool RawEos::sdk_accept(rust::Str user){std::lock_guard lock(sdk_mutex);
  const std::string text(user);auto id=EOS_ProductUserId_FromString(text.c_str());if(!id||!impl_->request)return false;
  EOS_P2P_AcceptConnectionOptions options{};options.ApiVersion=EOS_P2P_ACCEPTCONNECTION_API_LATEST;
  options.LocalUserId=impl_->local;options.RemoteUserId=id;options.SocketId=&impl_->socket;
  return EOS_P2P_AcceptConnection(impl_->p2p,&options)==EOS_Success;
}
bool RawEos::sdk_send(rust::Str user,rust::Slice<const std::uint8_t> bytes,bool delayed){std::lock_guard lock(sdk_mutex);
  if(bytes.empty()||bytes.size()>EOS_P2P_MAX_PACKET_SIZE||!impl_->request)return false;
  const std::string text(user);auto id=EOS_ProductUserId_FromString(text.c_str());if(!id)return false;
  EOS_P2P_SendPacketOptions options{};options.ApiVersion=EOS_P2P_SENDPACKET_API_LATEST;
  options.LocalUserId=impl_->local;options.RemoteUserId=id;options.SocketId=&impl_->socket;
  options.Data=bytes.data();options.DataLengthBytes=static_cast<uint32_t>(bytes.size());
  options.Reliability=EOS_PR_UnreliableUnordered;options.bAllowDelayedDelivery=delayed?EOS_TRUE:EOS_FALSE;options.bDisableAutoAcceptConnection=EOS_TRUE;
  return EOS_P2P_SendPacket(impl_->p2p,&options)==EOS_Success;
}
void RawEos::sdk_session_create(const EosSession &session,std::uint32_t capacity){std::lock_guard lock(sdk_mutex);
  if(!impl_->local||impl_->session_active||capacity<1||capacity>16)throw std::runtime_error("Invalid EOS session lifecycle");
  const auto bucket=std::string(session.playlist)+":"+std::string(session.revision)+":halo2-native-queue-v13";
  EOS_Sessions_CreateSessionModificationOptions create{};create.ApiVersion=EOS_SESSIONS_CREATESESSIONMODIFICATION_API_LATEST;
  create.SessionName=impl_->session_name.c_str();create.BucketId=bucket.c_str();create.MaxPlayers=capacity;
  create.LocalUserId=impl_->local;create.SessionId=impl_->socket.SocketName;
  EOS_HSessionModification handle{};check(EOS_Sessions_CreateSessionModification(impl_->sessions,&create,&handle));
  std::unique_ptr<std::remove_pointer_t<EOS_HSessionModification>,decltype(&EOS_SessionModification_Release)> modification(handle,EOS_SessionModification_Release);
  EOS_SessionModification_SetPermissionLevelOptions permission{};permission.ApiVersion=EOS_SESSIONMODIFICATION_SETPERMISSIONLEVEL_API_LATEST;
  permission.PermissionLevel=EOS_EOnlineSessionPermissionLevel::EOS_OSPF_InviteOnly;
  check(EOS_SessionModification_SetPermissionLevel(handle,&permission));
  auto attribute=[&](const char *key,const rust::String &value){
    const std::string text(value);
    EOS_Sessions_AttributeData data{};data.ApiVersion=EOS_SESSIONS_ATTRIBUTEDATA_API_LATEST;data.Key=key;
    data.ValueType=EOS_ESessionAttributeType::EOS_SAT_String;data.Value.AsUtf8=text.c_str();
    EOS_SessionModification_AddAttributeOptions options{};options.ApiVersion=EOS_SESSIONMODIFICATION_ADDATTRIBUTE_API_LATEST;
    options.SessionAttribute=&data;options.AdvertisementType=EOS_ESessionAttributeAdvertisementType::EOS_SAAT_Advertise;
    check(EOS_SessionModification_AddAttribute(handle,&options));
  };
  attribute("H2_PLAYLIST",session.playlist);attribute("H2_REVISION",session.revision);
  attribute("H2_MAP",session.map);attribute("H2_VARIANT",session.variant);
  EOS_Sessions_UpdateSessionOptions update{};update.ApiVersion=EOS_SESSIONS_UPDATESESSION_API_LATEST;update.SessionModificationHandle=handle;
  EOS_Sessions_UpdateSession(impl_->sessions,&update,impl_.get(),[](const EOS_Sessions_UpdateSessionCallbackInfo *data){
    if(!EOS_EResult_IsOperationComplete(data->ResultCode))return;
    auto &self=*static_cast<Impl*>(data->ClientData);self.session_active=data->ResultCode==EOS_Success;
    if(data->ResultCode!=EOS_Success){self.event(10,outcome(data->ResultCode));return;}
    // The owner must register before inviting. Creation alone does not do this
    // (the official SessionMatchmaking sample follows the same lifecycle).
    EOS_Sessions_RegisterPlayersOptions registration{};registration.ApiVersion=EOS_SESSIONS_REGISTERPLAYERS_API_LATEST;
    registration.SessionName=self.session_name.c_str();registration.PlayersToRegister=&self.local;registration.PlayersToRegisterCount=1;
    EOS_Sessions_RegisterPlayers(self.sessions,&registration,&self,[](const EOS_Sessions_RegisterPlayersCallbackInfo *registered){
      if(!EOS_EResult_IsOperationComplete(registered->ResultCode))return;
      auto &owner=*static_cast<Impl*>(registered->ClientData);
      bool admitted=false;
      for(std::uint32_t i=0;i<registered->RegisteredPlayersCount;++i)admitted|=registered->RegisteredPlayers[i]==owner.local;
      owner.event(10,outcome(registered->ResultCode==EOS_Success&&!admitted?EOS_AccessDenied:registered->ResultCode));
    });
  });
}
void RawEos::sdk_session_invite(rust::Str user){std::lock_guard lock(sdk_mutex);
  const std::string text(user);auto remote=EOS_ProductUserId_FromString(text.c_str());
  if(!impl_->session_active||!remote)throw std::runtime_error("Invalid EOS session invitation");
  EOS_Sessions_SendInviteOptions invite{};invite.ApiVersion=EOS_SESSIONS_SENDINVITE_API_LATEST;
  invite.SessionName=impl_->session_name.c_str();invite.LocalUserId=impl_->local;invite.TargetUserId=remote;
  EOS_Sessions_SendInvite(impl_->sessions,&invite,impl_.get(),[](const EOS_Sessions_SendInviteCallbackInfo *data){
    if(!EOS_EResult_IsOperationComplete(data->ResultCode))return;
    static_cast<Impl*>(data->ClientData)->event(data->ResultCode==EOS_Success?14:13,outcome(data->ResultCode));});
}
EosInviteInfo RawEos::sdk_session_info(rust::Str invite)const{std::lock_guard lock(sdk_mutex);
  const std::string text(invite);EOS_Sessions_CopySessionHandleByInviteIdOptions copy{};copy.ApiVersion=EOS_SESSIONS_COPYSESSIONHANDLEBYINVITEID_API_LATEST;copy.InviteId=text.c_str();
  EOS_HSessionDetails handle{};check(EOS_Sessions_CopySessionHandleByInviteId(impl_->sessions,&copy,&handle));
  std::unique_ptr<std::remove_pointer_t<EOS_HSessionDetails>,decltype(&EOS_SessionDetails_Release)> details(handle,EOS_SessionDetails_Release);
  EOS_SessionDetails_CopyInfoOptions info{};info.ApiVersion=EOS_SESSIONDETAILS_COPYINFO_API_LATEST;
  EOS_SessionDetails_Info *raw{};check(EOS_SessionDetails_CopyInfo(handle,&info,&raw));
  std::unique_ptr<EOS_SessionDetails_Info,decltype(&EOS_SessionDetails_Info_Release)> owned(raw,EOS_SessionDetails_Info_Release);
  EosInviteInfo result{};result.session=raw->SessionId?raw->SessionId:"";
  if(raw->OwnerUserId)result.owner=user_text(raw->OwnerUserId);
  auto attribute=[&](const char *key){
    EOS_SessionDetails_CopySessionAttributeByKeyOptions options{};options.ApiVersion=EOS_SESSIONDETAILS_COPYSESSIONATTRIBUTEBYKEY_API_LATEST;options.AttrKey=key;
    EOS_SessionDetails_Attribute *value{};check(EOS_SessionDetails_CopySessionAttributeByKey(handle,&options,&value));
    std::unique_ptr<EOS_SessionDetails_Attribute,decltype(&EOS_SessionDetails_Attribute_Release)> data(value,EOS_SessionDetails_Attribute_Release);
    if(!value->Data||value->Data->ValueType!=EOS_ESessionAttributeType::EOS_SAT_String||!value->Data->Value.AsUtf8)throw std::runtime_error("Invalid EOS session metadata");
    return std::string(value->Data->Value.AsUtf8);
  };
  result.playlist=attribute("H2_PLAYLIST");result.revision=attribute("H2_REVISION");result.map=attribute("H2_MAP");result.variant=attribute("H2_VARIANT");return result;
}
void RawEos::sdk_session_query(){std::lock_guard lock(sdk_mutex);
  if(!impl_->local||impl_->invite_query_pending)return;
  impl_->invite_query_pending=true;
  EOS_Sessions_QueryInvitesOptions options{};options.ApiVersion=EOS_SESSIONS_QUERYINVITES_API_LATEST;options.LocalUserId=impl_->local;
  EOS_Sessions_QueryInvites(impl_->sessions,&options,impl_.get(),[](const EOS_Sessions_QueryInvitesCallbackInfo *data){
    if(!EOS_EResult_IsOperationComplete(data->ResultCode))return;
    auto &self=*static_cast<Impl*>(data->ClientData);self.invite_query_pending=false;
    self.event(15,outcome(data->ResultCode));
    if(data->ResultCode!=EOS_Success)return;
    EOS_Sessions_GetInviteCountOptions count{};count.ApiVersion=EOS_SESSIONS_GETINVITECOUNT_API_LATEST;count.LocalUserId=self.local;
    const auto size=std::min<std::uint32_t>(EOS_Sessions_GetInviteCount(self.sessions,&count),16);
    for(std::uint32_t i=0;i<size;++i){
      EOS_Sessions_GetInviteIdByIndexOptions item{};item.ApiVersion=EOS_SESSIONS_GETINVITEIDBYINDEX_API_LATEST;item.LocalUserId=self.local;item.Index=i;
      char invite[EOS_SESSIONS_INVITEID_MAX_LENGTH+1]{};int32_t length=sizeof(invite);
      if(EOS_Sessions_GetInviteIdByIndex(self.sessions,&item,invite,&length)!=EOS_Success)continue;
      self.invitation(invite);
    }
  });
}
void RawEos::sdk_session_register(rust::Str user){std::lock_guard lock(sdk_mutex);
  const std::string text(user);auto remote=EOS_ProductUserId_FromString(text.c_str());
  if(!impl_->session_active||!remote)throw std::runtime_error("Invalid EOS session registration");
  EOS_Sessions_RegisterPlayersOptions options{};options.ApiVersion=EOS_SESSIONS_REGISTERPLAYERS_API_LATEST;
  options.SessionName=impl_->session_name.c_str();options.PlayersToRegister=&remote;options.PlayersToRegisterCount=1;
  EOS_Sessions_RegisterPlayers(impl_->sessions,&options,impl_.get(),[](const EOS_Sessions_RegisterPlayersCallbackInfo *data){
    if(!EOS_EResult_IsOperationComplete(data->ResultCode))return;
    auto &self=*static_cast<Impl*>(data->ClientData);
    if(data->ResultCode!=EOS_Success||data->RegisteredPlayersCount!=1){
      self.event(13,outcome(data->ResultCode==EOS_Success?EOS_AccessDenied:data->ResultCode));return;
    }
    self.event(16,0,data->RegisteredPlayers[0]);
  });
}
void RawEos::sdk_session_join(rust::Str invite){std::lock_guard lock(sdk_mutex);
  const std::string text(invite);EOS_Sessions_CopySessionHandleByInviteIdOptions copy{};copy.ApiVersion=EOS_SESSIONS_COPYSESSIONHANDLEBYINVITEID_API_LATEST;copy.InviteId=text.c_str();
  EOS_HSessionDetails handle{};check(EOS_Sessions_CopySessionHandleByInviteId(impl_->sessions,&copy,&handle));
  std::unique_ptr<std::remove_pointer_t<EOS_HSessionDetails>,decltype(&EOS_SessionDetails_Release)> details(handle,EOS_SessionDetails_Release);
  EOS_Sessions_JoinSessionOptions join{};join.ApiVersion=EOS_SESSIONS_JOINSESSION_API_LATEST;join.SessionName=impl_->session_name.c_str();join.SessionHandle=handle;join.LocalUserId=impl_->local;
  EOS_Sessions_JoinSession(impl_->sessions,&join,impl_.get(),[](const EOS_Sessions_JoinSessionCallbackInfo *data){
    if(!EOS_EResult_IsOperationComplete(data->ResultCode))return;
    auto &self=*static_cast<Impl*>(data->ClientData);self.session_active=data->ResultCode==EOS_Success;self.event(11,outcome(data->ResultCode));});
}
bool eos_sdk_available()noexcept{return true;}
#else
struct RawEos::Impl {};
RawEos::RawEos(const EosConfig &){throw std::runtime_error("This build has no EOS SDK. Configure HALO2_EOS_SDK_ROOT or HALO2_EOS_SDK_ARCHIVE.");}
RawEos::~RawEos()=default;
void RawEos::sdk_device(){}
void RawEos::sdk_login(){}
void RawEos::sdk_external_login(rust::Str){}
rust::String RawEos::sdk_id_token()const{return {};}
void RawEos::sdk_create_user(){}
void RawEos::sdk_socket(rust::Str,bool){}
rust::Vec<EosEvent> RawEos::sdk_poll(){return {};}
bool RawEos::sdk_accept(rust::Str){return false;}
bool RawEos::sdk_send(rust::Str,rust::Slice<const std::uint8_t>,bool){return false;}
void RawEos::sdk_session_create(const EosSession &,std::uint32_t){}
void RawEos::sdk_session_invite(rust::Str){}
void RawEos::sdk_session_query(){}
void RawEos::sdk_session_register(rust::Str){}
EosInviteInfo RawEos::sdk_session_info(rust::Str)const{return {};}
void RawEos::sdk_session_join(rust::Str){}
bool eos_sdk_available()noexcept{return false;}
#endif
std::unique_ptr<RawEos> new_raw_eos(const EosConfig &config){return std::make_unique<RawEos>(config);}
}
