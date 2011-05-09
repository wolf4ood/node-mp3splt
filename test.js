var splitter = require("./build/default/binding");
var hello = new splitter.Splitter();

hello.split("red.mp3","0.1","1.1",function(name){
	console.log(name);
});

