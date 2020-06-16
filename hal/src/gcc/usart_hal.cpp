/* Includes ------------------------------------------------------------------*/
#include "usart_hal.h"
#include "socket_hal.h"

struct Usart {
    virtual void init(hal_usart_ring_buffer_t *rx_buffer, hal_usart_ring_buffer_t *tx_buffer)=0;
    virtual void begin(uint32_t baud)=0;
    virtual void end()=0;
    virtual int32_t available()=0;
    virtual int32_t availableForWrite()=0;
    virtual int32_t read()=0;
    virtual int32_t peek()=0;
    virtual uint32_t write(uint8_t byte)=0;
    virtual void flush()=0;

    bool enabled() { return true; }
};

const sock_handle_t SOCKET_INVALID = sock_handle_t(-1);

class SocketUsartBase : public Usart
{
    private:
        hal_usart_ring_buffer_t* rx;
        hal_usart_ring_buffer_t* tx;

    protected:
        sock_handle_t socket;


        SocketUsartBase() : socket(SOCKET_INVALID) {}

        virtual bool initSocket()=0;

        inline int32_t read_char(bool peek=false)
        {
            if (rx->tail==rx->head)
                return -1;

            int32_t c = rx->buffer[rx->tail];
            if (!peek)
                rx->tail = (rx->tail+1) % SERIAL_BUFFER_SIZE;
            return c;
        }

        inline void write_char(unsigned char c)
        {
            unsigned i = (unsigned int)(tx->head + 1) % SERIAL_BUFFER_SIZE;
            if (i != tx->tail)
            {
                tx->buffer[tx->head] = c;
                tx->head = i;
            }
        }

        void fillFromSocketIfNeeded() {
            int space;
            if (rx->head>rx->tail) {    // head after tail, so can fill up to end of buffer
                space = SERIAL_BUFFER_SIZE-rx->tail;
            }
            else {
                space = rx->tail-rx->head;  // may be 0
            }
            if (socket!=SOCKET_INVALID && space>0) {
                socket_receive(socket, rx->buffer+rx->head, space, 0);
            }
        }


    public:
        virtual void init(hal_usart_ring_buffer_t *rx_buffer, hal_usart_ring_buffer_t *tx_buffer) override
        {
            this->rx = rx_buffer;
            this->tx = tx_buffer;
        }

        virtual void end() override {
            socket_close(socket);
        }
        virtual void flush() override {
            // todo
        }

        virtual int32_t available() override {
            fillFromSocketIfNeeded();
            return (rx->head-rx->tail) % SERIAL_BUFFER_SIZE;
        }
        virtual int32_t availableForWrite() override {
            return (SERIAL_BUFFER_SIZE + tx->head - tx->tail) % SERIAL_BUFFER_SIZE;
        }
        virtual int32_t read() override {
            fillFromSocketIfNeeded();
            return read_char();
        }
        virtual int32_t peek() override {
            fillFromSocketIfNeeded();
            return read_char(true);
        }
        virtual uint32_t write(uint8_t byte) override {
            if (!initSocket())
                return 0;
            return socket_send(socket, &byte, 1);
        }
};



/**
 * Client that provides data to/from the server when connected.
 */
class SocketUsartClient : public SocketUsartBase {

    virtual bool initSocket() {
        if (socket==SOCKET_INVALID) {
            socket = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, 54, 0);

            sockaddr_t socketAddr;
            int testResult = 0;

            // the family is always AF_INET
            socketAddr.sa_family = AF_INET;

            socketAddr.sa_data[0] = 0;
            socketAddr.sa_data[1] = 54;

            // the destination IP address: 8.8.8.8
            socketAddr.sa_data[2] = 127;
            socketAddr.sa_data[3] = 0;
            socketAddr.sa_data[4] = 0;
            socketAddr.sa_data[5] = 1;

            testResult = socket_connect(socket, &socketAddr, sizeof(socketAddr));
            if (testResult) {
                socket_close(socket);
                socket = SOCKET_INVALID;
            }
        }
        return socket!=SOCKET_INVALID;
    }

    public:

        virtual void begin(uint32_t baud) {}

};

/**
 * Client that provides data to/from the server when connected.
 */
class SocketUsartServer : public SocketUsartBase {

    protected:

        virtual bool initSocket() {
            return socket!=SOCKET_INVALID;
        }

    public:

        virtual void begin(uint32_t baud) {}
};


Usart& usartMap(unsigned index) {
#if defined(SPARK_TEST_DRIVER) && SPARK_TEST_DRIVER==1
static SocketUsartServer usart1 = SocketUsartServer();
static SocketUsartServer usart2 = SocketUsartServer();
#else
static SocketUsartClient usart1 = SocketUsartClient();
static SocketUsartClient usart2 = SocketUsartClient();
#endif

    switch (index) {
        case 0: return usart1;
        default: return usart2;

    }

}

void hal_usart_init(hal_usart_interface_t serial, hal_usart_ring_buffer_t *rx_buffer, hal_usart_ring_buffer_t *tx_buffer)
{
    usartMap(serial).init(rx_buffer, tx_buffer);
}

void hal_usart_begin(hal_usart_interface_t serial, uint32_t baud)
{
    //usartMap(serial).begin(baud);
}

void hal_usart_end(hal_usart_interface_t serial)
{
    //usartMap(serial).end();
}

int32_t hal_usart_available_data_for_write(hal_usart_interface_t serial)
{
    return usartMap(serial).availableForWrite();
}

uint32_t hal_usart_write(hal_usart_interface_t serial, uint8_t data)
{
    return usartMap(serial).write(data);
}

int32_t hal_usart_available(hal_usart_interface_t serial)
{
    return usartMap(serial).available();
}

int32_t hal_usart_read(hal_usart_interface_t serial)
{
    return usartMap(serial).read();
}

int32_t hal_usart_peek(hal_usart_interface_t serial)
{
    return usartMap(serial).peek();
}

void hal_usart_flush(hal_usart_interface_t serial)
{
    usartMap(serial).flush();
}

bool hal_usart_is_enabled(hal_usart_interface_t serial)
{
    return usartMap(serial).enabled();
}

void hal_usart_half_duplex(hal_usart_interface_t serial, bool Enable)
{
}

void hal_usart_begin_config(hal_usart_interface_t serial, uint32_t baud, uint32_t config, void *ptr)
{
}

uint32_t hal_usart_write_nine_bits(hal_usart_interface_t serial, uint16_t data)
{
    return usartMap(serial).write((uint8_t) data);
}

void hal_usart_send_break(hal_usart_interface_t serial, void* reserved)
{
}

uint8_t hal_usart_break_detected(hal_usart_interface_t serial)
{
  return 0;
}