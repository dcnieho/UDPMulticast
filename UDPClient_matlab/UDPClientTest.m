% testing MATLAB class wrapper to underlying C++ class UDPClient

udp = UDPClient();
udp.setPort(4444);
udp.getGroupAddress()
udp.setUseWTP(true);
udp.setMaxClockRes(true);
udp.init();
udp.setLoopBack(true);

% send a bunch of messages
for i=1:2048
    udp.sendWithTimeStamp('dat,1,crap',',');
end
dataMsgs = udp.getData;
for i=1:20
    udp.sendWithTimeStamp('dat,2,crap');    % testing default delimiter
end
dataMsgs2 = udp.getData;


% send a bunch of commands
for i=1:10
    udp.send('cmd');  % testing empty string in message struct
end
cmdMsgs = udp.getCommands;

% send exit msg
fprintf('nThreads active: %i\n',udp.checkReceiverThreads);
udp.send('exit');
fprintf('nThreads active: %i\n',udp.checkReceiverThreads);

% clean up
udp.deInit();
fprintf('nThreads active: %i\n',udp.checkReceiverThreads);
