var context;//初始化
function init() {
    if (typeof qt != 'undefined ') {
        new QwebChannel(qt.webChannelTransport, function (channel) {
            context = channel.objects.context;
        }
        );
    }
    else {
        alert("qt object fail");
    }
}
//向qt发送消息
function sendMessage(msg) {
    if (typeof context == "undefined") {
        alert("context object fail");
    }
    else {
        context.onMsg(msg);
    }
}
//控件控制函数
function onBtnSendMsg() {
    var cmd = document.getElementById("Message to be sent").value;
    sendMessage(cmd);
}
//接收qt发送的消息
function recvMessage(msg) {
    alert("Received msg by Qt:" + msg);
}

function displayString(string) {
    var element = document.getElementById('output');
    element.textContent = string;
  }
  

init();
displayString('strindg');