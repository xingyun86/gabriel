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
 *   @date: 2013-11-29 09:00:34                                        *
 *                                                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef GABRIEL__BASE__SERVER
#define GABRIEL__BASE__SERVER

#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "gabriel/base/connector.hpp"
#include "gabriel/base/acceptor.hpp"
#include "gabriel/base/thread.hpp"
#include "gabriel/base/client_connection.hpp"
#include "gabriel/base/server_connection.hpp"

namespace gabriel {
namespace base {
    
class Server : public Entity_Manager<Client_Connection, KEY_ID, true>
{
public:
    Server();
    virtual ~Server();
    void add_connection(Client_Connection *client_connection);
    virtual bool verify_connection(Client_Connection *client_connection);
    void main();
    uint32 state() const;
    void state(uint32 _state);
    virtual void on_connection_shutdown(Server_Connection *server_connection);
    virtual void on_connection_shutdown(Client_Connection *client_connection);
    virtual void handle_client_connection_msg(Client_Connection *client_connection, uint32 msg_type, uint32 msg_id, void *data, uint32 size);
    
protected:
    void register_client_connection_msg_handler(uint32 msg_type, uint32 msg_id, void (*handler)(Client_Connection *client_connection, void *data, uint32 size));
    Gabriel_Acceptor<Client_Connection, ACE_SOCK_ACCEPTOR> m_acceptor;
    Gabriel_Connector<Server_Connection, ACE_SOCK_CONNECTOR> m_connector;
    
private:    
    int32 init();
    void fini();
    void run();
    void do_reactor();
    void do_decode();
    void do_encode();
    void do_main();
    void do_main_client_connection();
    virtual void do_decode_server_connection() = 0;
    virtual void do_encode_server_connection() = 0;
    virtual void do_main_server_connection() = 0;
    virtual int32 init_hook() = 0;
    virtual void update();
    virtual void fini_hook();
    ID_Allocator<> m_connection_id_allocator;
    uint32 m_state;
    Thread<Server> m_thread;
    Message_Handler<Client_Connection> m_client_connection_msg_handler;
};

}
}

#endif
