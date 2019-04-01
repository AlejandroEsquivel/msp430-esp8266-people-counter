/**
 * @ngdoc function
 * @name tcpFrontendApp.controller:MainCtrl
 * @description
 * # MainCtrl
 * Controller of the tcpFrontendApp
 */

const PayloadTypes = {
  'CONNECTION_STATUS': 'CONNECTION_STATUS',
  'MESSAGE': 'MESSAGE',
  'COUNT_UPDATE': 'COUNT_UPDATE'
}

const directionTypes = { 
  "INCREASE": "+",
  "DECREASE": "-"
};

angular.module('tcpFrontendApp')
  .controller('MainCtrl', function ($timeout, $websocket, $scope) {
    var _self = this;

    _self.state = {
      count: 0,
      tcpConnection: false
    }

    const WS_HOST = 3200;
    const prompt = 'socket>';

    let ws = $websocket(`ws://localhost:${WS_HOST}`,null, { reconnectIfNotNormalClose: true });
    let wsRetry; 

    //-----Terminal Settings

    _self.echo = (msg) => _self.term.echo(`${prompt} ${msg}`);

    _self.onCommand = function (command) {
      if (command !== '') {
        try {
          var result = window.eval(command);
          if (result !== undefined) {
            this.echo(new String(result));
          }
        } catch (e) {
          this.error(new String(e));
        }
      } else {
        this.echo('');
      }
    };

    _self.term = $('#jsterm').terminal(_self.onCommand, {
      greetings: 'ESP8266 TCP-to-Websocket Data\n',
      name: 'js_demo',
      height: 400,
      prompt
    });

    //-----END Terminal Settings

    //-----Websocket Handlers

    ws.onOpen (()=>{
      
    wsRetry && clearInterval(wsRetry);
      
      ws.onMessage((buf)=>{
        const payload = JSON.parse(buf.data);


        switch(payload.type){
          case PayloadTypes.MESSAGE: 
            _self.echo(payload.message);
          break;
          case PayloadTypes.CONNECTION_STATUS: 
            const prevState = _self.state.tcpConnection;
            _self.state.tcpConnection = payload.data.tcp > 0;

            if(_self.state.tcpConnection){
              // connection established
            } else if(prevState && !_self.state.tcpConnection){
              // connection terminated
            }
          break;
          case PayloadTypes.COUNT_UPDATE:
            if(payload.data === directionTypes.INCREASE){
              _self.echo("Someone walked in to the store.");
            }
            else if(payload.data === directionTypes.DECREASE){
              _self.echo("Someone walked out of the store.");
            }
            _self.onRecieveCounterUpdate(payload.data);
          break;
        }

        console.log('Recieved ws payload', payload);
      });
    })

    ws.onClose(()=>{
      /*wsRetry = setInterval(function(){
        console.log('Finding new connection')
        ws = $websocket(`ws://localhost:${WS_HOST}`,null, { reconnectIfNotNormalClose: true });
      },1000)*/
    });

    //--- END Websocket Handlers

    _self.onRecieveCounterUpdate = (dir)=>{
      if(dir === directionTypes.INCREASE){

        _self.state.count++;
        _self.appendToData(_self.state.count);
        
      } else if(dir === directionTypes.DECREASE && _self.state.count !== 0){

        _self.state.count--;
        _self.appendToData(_self.state.count);
      }
    }

    //--- Plot configuration

    const assignX = ()=> moment().format('hh:mm:ss a');

    _self.appendToData = (el)=>{
      const size = $scope.data[0].length;
      if(size <= 10){
        $scope.data[0].push(el);
        _self.appendToLabels();
      }
      else {
        $scope.data[0].shift();
        $scope.data[0].push(el);
        $scope.labels.shift();
        $scope.labels.push(assignX());
      }
    }
    
    _self.appendToLabels = ()=>{
      $scope.labels.push(assignX());
    }

    $scope.data = [[0]];
    $scope.labels = [assignX()];
    $scope.series = ['Telemetry Events'];

    _self.reset = ()=>{
      $scope.data = [[0]];
      $scope.labels = [assignX()];
      _self.state.count = 0;
    }
   
    $scope.onClick = function (points, evt) {
      console.log(points, evt);
    };

    $scope.options = {
      elements: {
        line: {
            tension: 0.1 // disables bezier curves
        }
      },
      scales: {
        yAxes: [
          {
            id: 'y-axis-1',
            type: 'linear',
            display: true,
            position: 'left',
            lineTension: 0,
            cubicInterpolationMode: ''
          }
        ]
      }
    };
    //--- END Plot configuration
  });

