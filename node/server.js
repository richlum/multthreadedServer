var net = require('net');
var counter=0;

var server = net.createServer(function (socket){
	socket.write('Echo Server\r\n');
	
	server.on('connect', function(socket.remoteAddress){
		console.log(socket.remoteAddress);
		counter++;
	}
	
	server.on('data', function(data){
		console.log(data.toString());
		recvd=data.toString();
		socket.write(recvd);
	});
	counter--;
});

server.listen(9998,'127.0.0.1');

