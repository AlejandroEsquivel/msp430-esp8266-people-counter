/*

This script only anticipates one active connection
as there is only one instance of the ESP8266 circuit

*/

const Net = require('net');
const WebSocket = require('ws');

const TCP_PORT = 3100;
const WS_PORT = 3200;

const server = new Net.Server({
    allowHalfOpen: true
});
const wss = new WebSocket.Server({ port: WS_PORT });

const ConnectionStatus = {
    'START': 1,
    'END': 0
}

const PayloadTypes = {
    'CONNECTION_STATUS': 'CONNECTION_STATUS',
    'MESSAGE': 'MESSAGE',
    'COUNT_UPDATE': 'COUNT_UPDATE'
}

let state = {
    connections: {
        web: 0,
        tcp: 0
    }
};

let websocket = {
    isAlive: false,
    connection: null,
    queue: [],
    send: payload => {
        if(websocket.connection !== null) {
            
            try {
                websocket.flush();
                websocket.connection.send(JSON.stringify(payload)); 
            }
            catch(e){
                console.warn('Websocket connection not available.')
            }
            
        }
        else {
            websocket.queue.push(payload);
        }
    },
    message: (message) => websocket.send({
        type: PayloadTypes.MESSAGE,
        message
    }),
    flush: () => {
        const size = websocket.queue.length;
        for(var i=0;i<size;i++){
            websocket.connection.send(JSON.stringify(websocket.queue.pop()));
        }
    },    
}

const sendConnectionStatus = () => {
    websocket.send({
        type: PayloadTypes.CONNECTION_STATUS,
        data: state.connections
    })
}

//-------- Main Handler

const onData = (chunk) => {

    let data = chunk.toString().replace(/(\r\n|\n|\r)/gm, ""); // remove newline

    websocket.send({
        type: PayloadTypes.COUNT_UPDATE,
        data
    });
}

//-------- Main Handler

const onTCPConnection = (status)=>{

    if(status===ConnectionStatus.START){
        state.connections.tcp++;
        websocket.message('A new connection has been established.');
    }
    else if(status===ConnectionStatus.END){
        state.connections.tcp--;
       websocket.message('Closing connection with the client.');
    }

    sendConnectionStatus();
}


wss.on('connection', function wsConnection(ws) {  
    websocket.connection = ws;
    sendConnectionStatus();
});

server.on('connection', function tcpConnection(socket) {
    onTCPConnection(ConnectionStatus.START);
    socket.on('data', onData);
    socket.on('end', ()=> onTCPConnection(ConnectionStatus.END));
    socket.on('error', err => console.log(`Error: ${err}`));
});

server.listen(TCP_PORT, ()=> console.log(`Server listening for connection requests on socket localhost:${TCP_PORT}\n`));
