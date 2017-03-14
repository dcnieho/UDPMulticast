import UDPClient

udp = UDPClient.UDPClient()
print "build from git revision ID " + udp.getGitRefID()
udp.port = 4444;
udp.init();
udp.loopBack = True;

if False:
    # test computer filter
    udp.setComputerFilter([1,2,3])

UDPClient.getCurrentTime()    # warmup timestamper

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
udp.send('cmd,you should see this');
cmdMsgs = udp.getCommands();

# test iterator
for x in cmdMsgs:
    print x.text

# send exit msg
print "nThreads active: " + str(udp.checkReceiverThreads())
udp.send('exit');
print "nThreads active: " + str(udp.checkReceiverThreads())

# clean up
udp.deInit();
print "nThreads active: " + str(udp.checkReceiverThreads())