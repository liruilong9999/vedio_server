var context;//��ʼ��
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
//��qt������Ϣ
function sendMessage(msg) {
    if (typeof context == "undefined") {
        alert("context object fail");
    }
    else {
        context.onMsg(msg);
    }
}
//�ؼ����ƺ���
function onBtnSendMsg() {
    var cmd = document.getElementById("Message to be sent").value;
    sendMessage(cmd);
}
//����qt���͵���Ϣ
function recvMessage(msg) {
    alert("Received msg by Qt:" + msg);
}

function displayString(string) {
    var element = document.getElementById('output');
    element.textContent = string;
  }
  

init();
displayString('strindg');