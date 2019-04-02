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
    connections: [],
    queue: [],
    send: payload => {
        let inactiveConnections = [];
        if(websocket.connections.length) {
            
            websocket.connections.forEach((connection,i)=>{
                try {
                    connection.send(JSON.stringify(payload)); 
                }
                catch(e){
                    console.warn(`Websocket connection ${i} not available.`);
                    inactiveConnections.push(i);
                }
            })

            inactiveConnections.forEach(i => websocket.connections.splice(i,1));
            
        }
    },
    message: (message) => websocket.send({
        type: PayloadTypes.MESSAGE,
        message
    })
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

    console.log(`Recieved on tcp socket: ${data}`);
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
    console.log(`Established websocket connection ${websocket.connections.length}`)
    websocket.connections.push(ws);
    sendConnectionStatus();
});

server.on('connection', function tcpConnection(socket) {

    onTCPConnection(ConnectionStatus.START);

    socket.on('data', onData);
    socket.on('end', ()=> onTCPConnection(ConnectionStatus.END));
    socket.on('error', err => console.log(`Error: ${err}`));
});

server.listen(TCP_PORT, ()=> console.log(`Server listening for tcp connection requests @ localhost:${TCP_PORT}\n`));
