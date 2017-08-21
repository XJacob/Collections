
/************* http server exapmle *********************/
/*
var http = require("http");
http.createServer(function(request, response){
    // Send the HTTP header
    // HTTP Status: 200 : OK
    // Content Type: text/plain
    response.writeHead(200, {'Content-Type': 'text/plain'});

    //
    // Send the response body as "Hello World"
    response.end('Hello World\n');
}).listen(8081);
*/
/*----------------------------------------------------*/

/************** read file example ********************/
var fs = require("fs");
/* blocking read */
var data = fs.readFileSync('input.txt');
console.log(data.toString());
/* nonblocking read */
fs.readFile('input.txt', function(err, data){
    if (err) return console.error(err);
    console.log(data.toString());
});
/*----------------------------------------------------*/

/************** event example ********************/
var events = require("events");
var eventEmitter = new events.EventEmitter();

var event1Handler = function print_event(arg1, arg2, arg3) {
    console.log("event1 trigger: " + arg1 + arg2 + arg3);
    console.log(arguments);
    eventEmitter.emit('event2');
}

var event1Handler_1 = function print_event() {
    console.log("event1-1 trigger");
}

eventEmitter.on('event2', function(){
    console.log("event2 trigger");
});

x = 000
var y = 999
console.log("The first line" + x + y);
eventEmitter.on('event1', event1Handler_1); //bind two event on the event1
eventEmitter.on('event1', event1Handler);
eventEmitter.emit('event1', "hello", "world", x, y);
/*----------------------------------------------------*/

/************** buffer example ********************/
console.log("\n\n\n");
buf1 = new Buffer(256);
buf2 = new Buffer([0x10,0x20,0x30,0x40,0x50]);

len = buf1.write("Simply Easy Learning");
buf2[3] = 0xaa;

var buf3 = Buffer.concat([buf1, buf2]);
if (buf3.compare(buf2) != 0) {
    console.log("buf3 is not the same as buf2.");
}

console.log("buf1: " + buf1.toString(undefined, 0, len));
console.log("buf2: "+  buf2.toString('hex', 0, 5));
console.log("buf3: "+  buf3.toString(undefined, 0, len) + " " + buf3.toString('hex', 256, 256+5));

/*----------------------------------------------------*/

/************** streaming example ********************/
var zlib = require('zlib');
var readerStream = fs.createReadStream('input.txt');
var data=''
readerStream.on('data', function(chunk) {
    data += chunk
});

readerStream.on('end',function(){
    console.log(data);
});

readerStream.on('error', function(err){
    console.log(err.stack);
});

fs.createReadStream('input.txt')
    .pipe(zlib.createGzip())
    .pipe(fs.createWriteStream('input.txt.gz'));

/*----------------------------------------------------*/

/************** timer example ********************/
console.log("\n\n\n");
var i = 0;
var interval_id;
var printTimeOut1 = function() {
    console.log("Time out!!");
}
var t1 = setTimeout(printTimeOut1, 1000);

interval_id = setInterval(function() {
    i++;
    console.log("Timer interval!  times: " + i);
    if ( i > 8)
        clearInterval(interval_id);
}, 500);

/*----------------------------------------------------*/

/************** child example ********************/
console.log("\n\n\n");
var child_process = require("child_process");
var process1 = child_process.exec("ls /", function(err, stdout, stderr){
    console.log(stdout);
    console.log(stderr);
});
process1.on('exit', function(code){
    console.log("process1 exit code : " + code);
});
/*----------------------------------------------------*/

/************** global variable example ********************/
console.log("\n\n\n");
console.log("path name: " + __dirname);
console.log("file name: " + __filename);
/*----------------------------------------------------*/
console.log("Program Ended\n\n\n");
