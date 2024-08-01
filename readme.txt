1. Total implementation time: ~24h

2. Test passed: All

3. Protocols explained

    a) PO_UDP - Protocol over UDP 
        - Used in comunications between UDP clients and the server
        - Used in the implementation of the PO_TCP
        - Fields of the message:
            char topic[50] -- Topic of the message
            uint8_t data_type -- Data type of the message
            char content[1500] -- The actual content
        - The data types that can be sent are:
            TYPE_INT -- 0
            TYPE_SHORT_REAL -- 1
            TYPE_FLOAT -- 2
            TYPE_STRING -- 3

    b) PO_TCP - Protocol over TCP -- IMPLEMENTED BY ME
        - Used in comunication between TCP clients and the server
        - Fields of the message:
            uint8_t op_code -- The operation code -- more on this below
            char udp_client_ip[16] -- UDP client's IP -- used only for "POST"
            uint16_t udp_client_port -- UDP client's PORT -- used only for "POST"
            struct udp_message message -- Actual message -- used only for "POST"
            char topic[50] -- Topic -- used only for "SUBSCRIBE", "UNSUBSCRIBE"
            char id[10]; -- TCP Client's ID -- used only for "CONNECT", "DISCONNECT"
        - Operation codes and their usage:
            0 :: SUBSCRIBE
                => When a TCP client wants to connect to se server, it needs to send a
                PO_TCP message with the "op_code" set as 0 and the "topic" completed with
                the topic he wants to subscribe to. After that the client need to wait for 
                a confimation from the server.
            1 :: SUBSCRIBE_ACK
                => When the server succesfuly subscribes a new tcp client to a topic, it 
                sends a PO_TCP message with the "op_code" set as 1 to the client.
                => When a client receives a PO_TCP message with the "op_code" set as 1,
                he knows that he succesfuly subscribe to the topic received in the
                message at the "topic" field.
            2 :: UNSUBSCRIBE
                => Similar to SUBSCRIBE just with an "op_code" of 2.
            3 :: UNSUBSCRIBE_ACK
                => Similar to SUBSCRIBE_ACK just with an "op_code" of 3.
            4 :: POST
                => When the server receives a message from a UDP client, it creates a
                new PO_TCP message with the "op_code" set as 4, writes the "udp_client_ip"
                and "udp_client_port" fields and copies the PO_UDP message in the "message"
                field. My protocol passes the duty of interpreting the message to the
                client. The function that decodes an PO_UDP message sits on the client.
                Because of that we have the advantage that we can modify the way that the
                information is interpreted at the client level which gives the clients more
                flexibility.
                => When a client receives a PO_TCP message containing an "op_code" equal
                to 4, decodes and print it to his terminal based on the documentation 
                provided for PO_UDP.
            5 :: CONNECT
                => When a user wants to connect to the server, he has to send an PO_TCP
                message with the "op_code" set as 5 and the "id" field completed with
                the ID he wants to connect as. After that the client need to wait for a 
                confimation from the server.
            6 :: CONNECT_ACK
                => When the server succesfuly connects a new tcp client, it sends a
                PO_TCP message with the "op_code" set as 6 to the client that has
                just connected to the server.
                => When a client receives a PO_TCP message with the "op_code" set as 6,
                he knows that the connection was succesfuly made.
            7 :: DISCONNECT
                => When the server receives a PO_TCP message from a TCP client that contains the
                "op_code" 7, it disconnects the clint with the ID equal to the one received
                in the message in the field "id".
                => When a client receives a PO_TCP message with an "op_code" equal to 7 he knows
                that he needs to close the connection with the server. So when the server closes,
                it sends PO_TCP messages containing the "op_code" of 7 and the specific ID for
                every client to every TCP client that is connected.
        - At the server we keep information about current and past users as a vector of TCP_clients.
        The fields of a TCP_client are the following:
            char id[10] -- Client ID
            bool connected; -- Connection status
            char ip[16] -- IP
            uint16_t port -- PORT
            int sockfd -- Socket file descriptor
            vector<string> topics_subscribed -- Topics subscribed by the client

Thank you for your time!

╭━┳━╭━╭━╮╮
┃┈┈┈┣▅╋▅┫┃
┃┈┃┈╰━╰━━━━━━╮
╰┳╯┈┈┈┈┈┈┈┈┈◢▉◣
╲┃┈┈┈┈┈┈┈┈┈┈▉▉▉
╲┃┈┈┈┈┈┈┈┈┈┈◥▉◤
╲┃┈┈┈┈╭━┳━━━━╯
╲┣━━━━━━┫
