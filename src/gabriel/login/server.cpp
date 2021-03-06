/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *      _____       ___   _____   _____    _   _____   _               *
 *     /  ___|     /   | |  _  \ |  _  \  | | | ____| | |              *
 *     | |        / /| | | |_| | | |_| |  | | | |__   | |              *
 *     | |  _    / / | | |  _  { |  _  /  | | |  __|  | |              *
 *     | |_| |  / /  | | | |_| | | | \ \  | | | |___  | |___           *
 *     \_____/ /_/   |_| |_____/ |_|  \_\ |_| |_____| |_____|          *
 *                                                                     *
 *     gabriel is an angel from the Holy Bible, this engine is named   *
 *   gabriel, means bringing people good news. the goal of gabriel     *
 *   server engine is to help people to develop various online games,  *
 *   welcome you to join in.                                           *
 *                                                                     *
 *   @author: lichuan                                                  *
 *   @qq: 308831759                                                    *
 *   @email: 308831759@qq.com                                          *
 *   @site: www.lichuan.me                                             *
 *   @github: https://github.com/lichuan/gabriel                       *
 *   @date: 2014-01-09 12:48:05                                        *
 *                                                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <iostream>
#include "yaml-cpp/yaml.h"
#include "ace/Dev_Poll_Reactor.h"
#include "gabriel/base/db.hpp"
#include "gabriel/login/server.hpp"
#include "gabriel/protocol/error.pb.h"
#include "gabriel/protocol/client/default.pb.h"
#include "gabriel/protocol/client/msg_type.pb.h"
#include "gabriel/protocol/server/msg_type.pb.h"

using namespace std;

namespace gabriel {
namespace login {

Server::Server()
{
    type(gabriel::base::LOGIN_SERVER);
}

Server::~Server()
{
}

void Server::on_connection_shutdown(gabriel::base::Client_Connection *client_connection)
{
    clear_account_by_conn_id(client_connection->id());
}

bool Server::on_connection_shutdown(gabriel::base::Server_Connection *server_connection)
{
    return Super::on_connection_shutdown(server_connection);
}

void Server::do_main_on_server_connection()
{
    Super::do_main_on_server_connection();
}
    
bool Server::verify_connection(gabriel::base::Client_Connection *client_connection)
{
    client_connection->schedule_timer([=]() {
            client_connection->shutdown();
        }, 0, 300000);
    
    return true;
}

bool Server::init_hook()
{
    return Super::init_hook();
}

void Server::update_hook()
{
    
}

void Server::do_reconnect()
{
    Super::do_reconnect();
}

void Server::register_msg_handler()
{
    Super::register_msg_handler();
    using namespace placeholders;
    
    {
        using namespace gabriel::protocol::server;
        m_server_msg_handler.register_handler(DEFAULT_MSG_TYPE, REGISTER_ORDINARY_SERVER, bind(&Server::register_rsp_from, this, _1, _2, _3));
        m_server_msg_handler.register_handler(DEFAULT_MSG_TYPE, DB_TASK, bind(&Server::handle_db_msg, this, _1, _2, _3));
        m_server_msg_handler.register_handler(DEFAULT_MSG_TYPE, SYNC_ACCOUNT, bind(&Server::sync_account_rsp, this, _1, _2, _3));
    }
    
    {
        using namespace gabriel::protocol::client;
        m_client_msg_handler.register_handler(DEFAULT_MSG_TYPE, REGISTER_ACCOUNT, bind(&Server::handle_user_register, this, _1, _2, _3));
    }
}

void Server::init_reactor()
{
    delete ACE_Reactor::instance(new ACE_Reactor(new ACE_Dev_Poll_Reactor(10000, true), true), true);
}

void Server::sync_account_rsp(gabriel::base::Connection *connection, void *data, uint32 size)
{
    using namespace gabriel::protocol;
    using namespace gabriel::protocol::server;
    PARSE_FROM_ARRAY(Sync_Account_Rsp, msg, data, size);
    client::Login_Account_Rsp rsp;
    rsp.set_result(ERR_OK);
    rsp.set_key(msg.key());
    rsp.set_port(msg.port());
    rsp.set_addr(msg.addr());
    uint32 conn_id = msg.conn_id_2();
    base::Connection *user_conn = get_connetion(conn_id);
    clear_account_by_conn_id(conn_id);
    
    if(user_conn != NULL)
    {
        user_conn->send(client::DEFAULT_MSG_TYPE, client::LOGIN_ACCOUNT, rsp);
    }
}
    
void Server::handle_db_msg(gabriel::base::Connection *connection, void *data, uint32 size)
{
    using namespace gabriel::protocol::server;
    PARSE_FROM_ARRAY(DB_Task, msg, data, size);
    uint32 msg_type = msg.msg_type();
    uint32 msg_id = msg.msg_id();
    
    if(msg_type == DEFAULT_MSG_TYPE)
    {
        if(msg_id == FORWARD_USER_MSG)
        {
            PARSE_FROM_STRING(Forward_User_Msg, forward_msg, msg.msg_data());
            handle_forward_user_msg(forward_msg);
        }
    }
}

void Server::handle_forward_user_msg(gabriel::protocol::server::Forward_User_Msg &msg)
{
    using namespace gabriel::protocol;
    using namespace gabriel::protocol::client;
    uint32 msg_type = msg.msg_type();
    uint32 msg_id = msg.msg_id();
    uint32 conn_id = msg.conn_id();
    gabriel::base::Connection *connection = get_connetion(conn_id);

    if(connection == NULL)
    {
        return;
    }
    
    if(msg_type == DEFAULT_MSG_TYPE)
    {
        if(msg_id == REGISTER_ACCOUNT)
        {
            PARSE_FROM_STRING(Register_Account_Rsp, rsp, msg.msg_data());
            connection->send(DEFAULT_MSG_TYPE, REGISTER_ACCOUNT, rsp);
            clear_account_by_conn_id(conn_id);
        }
        else if(msg_id == LOGIN_ACCOUNT)
        {
            PARSE_FROM_STRING(Login_Account_Rsp, rsp, msg.msg_data());

            if(rsp.result() == ERR_OK)
            {
                auto iter = m_connection_account_map.find(conn_id);
                server::Get_One_Gateway msg;
                msg.set_account(iter->second);
                msg.set_conn_id(conn_id);
                m_center_connection.send(server::DEFAULT_MSG_TYPE, server::GET_ONE_GATEWAY, msg);
            }
            else
            {
                connection->send(DEFAULT_MSG_TYPE, LOGIN_ACCOUNT, rsp);
                clear_account_by_conn_id(conn_id);
            }
        }
    }
}

void Server::handle_user_login(gabriel::base::Connection *connection, void *data, uint32 size)
{
    using namespace gabriel::protocol;
    using namespace gabriel::protocol::client;
    PARSE_FROM_ARRAY(Login_Account, msg, data, size);
    const string &account = msg.account();

    if(account.empty() || msg.password().empty())
    {
        return;
    }

    if(m_login_accounts.find(account) != m_login_accounts.end())
    {
        return;
    }
    
    uint32 conn_id = connection->id();
    m_login_accounts.insert(account);
    m_connection_account_map.insert(make_pair(conn_id, account));
    forward_user_msg_to_superrecord(DEFAULT_MSG_TYPE, LOGIN_ACCOUNT, msg, conn_id);
}
    
void Server::handle_user_register(gabriel::base::Connection *connection, void *data, uint32 size)
{
    using namespace gabriel::protocol::client;    
    PARSE_FROM_ARRAY(Register_Account, msg, data, size);
    const string &account = msg.account();

    if(account.empty() || msg.password().empty())
    {
        return;
    }
    
    if(m_login_accounts.find(account) != m_login_accounts.end())
    {
        return;
    }
    
    uint32 conn_id = connection->id();
    m_login_accounts.insert(account);
    m_connection_account_map.insert(make_pair(conn_id, account));
    forward_user_msg_to_superrecord(DEFAULT_MSG_TYPE, REGISTER_ACCOUNT, msg, conn_id);
}

void Server::clear_account_by_conn_id(uint32 conn_id)
{
    auto iter = m_connection_account_map.find(conn_id);
    
    if(iter == m_connection_account_map.end())
    {
        return;
    }
    
    m_login_accounts.erase(iter->second);
    m_connection_account_map.erase(iter);
}

void Server::forward_user_msg_to_superrecord(uint32 msg_type, uint32 msg_id, google::protobuf::Message &msg, uint32 conn_id, uint32 seq)
{
    using namespace gabriel::protocol::server;
    gabriel::protocol::server::DB_Task task;
    task.set_seq(seq);
    task.set_pool_id(gabriel::base::DB_Handler_Pool::GAME_POOL);
    task.set_msg_type(DEFAULT_MSG_TYPE);
    task.set_msg_id(FORWARD_USER_MSG);
    Forward_User_Msg forward_msg;
    forward_msg.set_msg_type(msg_type);
    forward_msg.set_msg_id(msg_id);
    forward_msg.set_conn_id(conn_id);
    msg.SerializeToString(forward_msg.mutable_msg_data());
    forward_msg.SerializeToString(task.mutable_msg_data());
    m_superrecord_connection.send(DEFAULT_MSG_TYPE, DB_TASK, task);
}
    
void Server::register_rsp_from(gabriel::base::Connection *connection, void *data, uint32 size)
{
    if(id() > 0)
    {
        return;
    }
    
    using namespace gabriel::protocol::server;
    PARSE_FROM_ARRAY(Register_Ordinary_Rsp, msg, data, size);
    
    if(msg.info_size() != 2)
    {
        state(gabriel::base::SERVER_STATE::SHUTDOWN);
        cout << "error: information of this server received from center server is wrong" << endl;
        
        return;
    }

    for(int32 i = 0; i != msg.info_size(); ++i)
    {
        const auto &info = msg.info(i);

        if(info.server_type() == gabriel::base::LOGIN_SERVER)
        {
            id(info.server_id());
            
            if(m_acceptor.open(ACE_INET_Addr(info.port(), info.outer_addr().c_str()), ACE_Reactor::instance()) < 0)
            {
                state(gabriel::base::SERVER_STATE::SHUTDOWN);
                cout << "error: start login server failed" << endl;
            
                return;
            }

            cout << "start login server ok" << endl;
            set_proc_name_and_log_dir("gabriel_login_server___%u___%u", zone_id(), id());
        }
        else
        {
            gabriel::base::Server_Connection *tmp = &m_record_connection;

            if(m_connector.connect(tmp, ACE_INET_Addr(info.port(), info.inner_addr().c_str())) < 0)
            {
                cout << "error: connect to record server failed" << endl;
                state(gabriel::base::SERVER_STATE::SHUTDOWN);
            }
            else
            {
                cout << "connect to record server ok" << endl;
            }
        }
    }
}

void Server::fini_hook()
{
}

}
}

#include "gabriel/main.cpp"
