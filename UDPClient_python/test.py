import UDPClient_python

udp = UDPClient_python.UDPClient()
udp.port = 4444;
udp.init();
udp.loopBack = True;
# send a bunch of messages
for i in range(1, 2048):
    udp.sendWithTimeStamp('dat,1,crap',',');
dataMsgs = udp.getData();

for i in range(1, 20):
    udp.sendWithTimeStamp('dat,2,crap');    # testing default delimiter
dataMsgs2 = udp.getData();


# send a bunch of commands
for i in range(1, 10):
    udp.send('cmd');    # testing empty string in message struct
cmdMsgs = udp.getCommands();

# test iterator
for x in cmdMsgs:
    print x.text

# send exit msg
udp.send('exit');

# clean up
udp.deInit();