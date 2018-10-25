% MATLAB class wrapper to underlying C++ class UDPClient

classdef UDPClient < handle
    properties (GetAccess = private, SetAccess = private, Hidden = true, Transient = true)
        instanceHandle;         % integer handle to a class instance in MEX function
    end
    properties (GetAccess = protected, SetAccess = immutable, Hidden = false)
        mexClassWrapperFnc;     % the MEX function owning the class instances
    end
    properties (Access = private, Hidden = true)
        computerFilter = [];
        debugLevel = 0;
        
        parsedCommandBuffer = {};
    end
    properties (SetAccess = private)
        hasSMIIntegration;
        hasTobiiIntegration;
    end
    
    
    methods (Static = true)
        function mexFnc = checkMEXFnc(mexFnc)
            % Input function_handle or name, return valid handle or error
            
            % accept string or function_handle
            if ischar(mexFnc)
                mexFnc = str2func(mexFnc);
            end
            
            % validate MEX-file function handle
            % http://stackoverflow.com/a/19307825/2778484
            funInfo = functions(mexFnc);
            if exist(funInfo.file,'file') ~= 3  % status 3 is MEX-file
                error('UDPClient:invalidMEXFunction','Invalid MEX file: "%s".',funInfo.file);
            end
        end
    end
    methods (Access = protected, Sealed = true)
        function varargout = cppmethod(this, methodName, varargin)
            if isempty(this.instanceHandle)
                error('UDPClient:invalidHandle','No class handle. Did you call init yet?');
            end
            [varargout{1:nargout}] = this.mexClassWrapperFnc(methodName, this.instanceHandle, varargin{:});
        end
        
        function varargout = cppmethodGlobal(this, methodName, varargin)
            [varargout{1:nargout}] = this.mexClassWrapperFnc(methodName, varargin{:});
        end
    end
    methods
        %% Constructor - Create a new C++ class instance 
        function this = UDPClient()
            this.mexClassWrapperFnc = this.checkMEXFnc('UDPClient_matlab');
            
            this.instanceHandle     = this.cppmethodGlobal('new');
            
            this.hasSMIIntegration  = this.cppmethod('hasSMIIntegration');
            this.hasTobiiIntegration= this.cppmethod('hasTobiiIntegration');
        end
        
        %% Destructor - Destroy the C++ class instance
        function delete(this)
            this.cppmethod('deInit');
            this.cppmethod('delete');
        end

        %% methods
        function init(this)
            this.cppmethod('init');
        end
        function deInit(this)
            this.cppmethod('deInit');
        end
        function sendWithTimeStamp(this, varargin)
            % check if first input is numeric, take that as the number of
            % times to send the message
            if isnumeric(varargin{1})
                nSend = varargin{1};
                varargin(1) = [];
            else
                nSend = 1;
            end
            if bitand(this.debugLevel,2)
                fprintf('sent: %s\n',varargin{1});
            end
            % send message
            for p=1:nSend
                this.cppmethod('sendWithTimeStamp', varargin{:});
            end
        end
        function send(this, varargin)
            this.cppmethod('send', varargin{:});
        end
        % check if receiver threads are still running. They may close when
        % an exit message is received. function returns how many threads
        % are running
        function numRunning = checkReceiverThreads(this)
            numRunning = this.cppmethod('checkReceiverThreads');
        end
        
        % get the data and command messages received since the last call to this function
        function data = getData(this)
            data = this.cppmethod('getData');
        end
        function cmds = getCommands(this)
            cmds = this.cppmethod('getCommands');
        end
        % get data grouped by computer
        function data = getDataGrouped(this, computers)
            % get default filter if not specified
            if nargin<2
                computers = this.computerFilter;
            end
            % get data messages from C++ buffer
            rawData = this.getData();
            % parse into {ip, timestamps, data}, with possibly multiple
            % rows of timestamps and data per computer
            data = {};
            ips  = [];
            for p=1:length(rawData)
                % optionally, only process messages from specified
                % computers
                if ~isempty(computers) && ~any(rawData(p).ip==computers)
                    continue
                end
                % find where in cell array to put 
                qIp = rawData(p).ip==ips;
                if ~any(qIp)
                    ips(end+1)    = rawData(p).ip;
                    data(end+1,:) = {rawData(p).ip,int64([]),[]};
                    qIp           = rawData(p).ip==ips;
                end
                str    = rawData(p).text;
                commas = find(str==',');
                % moet los, anders krijg je doubles ookal zeg je %*f
                % eerst de timestamps
                str(commas(1):commas(end)-1) = '';
                timestamps = sscanf(str,'%ld,%ld');
                % dat de data
                gazeData   = sscanf(rawData(p).text(commas(1)+1:commas(end)-1),'%f,%f,%f,%f');
                % store
                data{qIp,2}(end+1,:) = [timestamps.' rawData(p).timeStamp];   % SMI timestamp, send timestamp, receive timestamp
                data{qIp,3}(end+1,:) = gazeData;  % leftX, leftY, rightX, rightY
            end
        end
        
        % get cmds, one at a time, filtered and parsed
        function cmd = getSingleCommand(this, computers)
            cmd = [];
            % get default filter if not specified
            if nargin<2
                computers = this.computerFilter;
            end
            % If our buffer is empty, see if any new commands have arrived
            if isempty(this.parsedCommandBuffer)
                this.fillCommandBuffer();
            end
            % see if there is any command to return
            if ~isempty(this.parsedCommandBuffer)
                % loop until we have a command that matches filter
                % things that do not match filter are discarded forever
                while true
                    if isempty(this.parsedCommandBuffer)
                        % nothing left, return empty
                        cmd = [];
                        return;
                    end
                    
                    % pop first-in command and return that
                    cmd = this.parsedCommandBuffer(1,:);
                    this.parsedCommandBuffer(1,:) = [];
                    % see if its a message we want
                    if isempty(computers) || any(cmd{1}==computers)
                        % matches filter, return it
                        return;
                    end
                end
            end
        end
        
        % getters and setters
        function gitRefID = getGitRefID(this)
            gitRefID = this.cppmethod('getGitRefID');
        end
        function setUseWTP(this, varargin)
            this.cppmethod('setUseWTP', varargin{:});
        end
        function setMaxClockRes(this, varargin)
            this.cppmethod('setMaxClockRes', varargin{:});
        end
        function loopback = getLoopBack(this)
            loopback = this.cppmethod('getLoopBack');
        end
        function setLoopBack(this, varargin)
            this.cppmethod('setLoopBack', varargin{:});
        end
        function reuseSocket = getReuseSocket(this)
            reuseSocket = this.cppmethod('getReuseSocket');
        end
        function setReuseSocket(this, varargin)
            this.cppmethod('setReuseSocket', varargin{:});
        end
        function groupAddress = getGroupAddress(this)
            groupAddress = this.cppmethod('getGroupAddress');
        end
        function setGroupAddress(this, varargin)
            this.cppmethod('setGroupAddress', varargin{:});
        end
        function port = getPort(this)
            port = this.cppmethod('getPort');
        end
        function setPort(this, varargin)
            this.cppmethod('setPort', varargin{:});
        end
        function bufferSize = getBufferSize(this)
            bufferSize = this.cppmethod('getBufferSize');
        end
        function setBufferSize(this, varargin)
            this.cppmethod('setBufferSize', varargin{:});
        end
        function numQueuedReceives = getNumQueuedReceives(this)
            numQueuedReceives = this.cppmethod('getNumQueuedReceives');
        end
        function setNumQueuedReceives(this, varargin)
            this.cppmethod('setNumQueuedReceives', varargin{:});
        end
        function numReceiverThreads = getNumReceiverThreads(this)
            numReceiverThreads = this.cppmethod('getNumReceiverThreads');
        end
        function setNumReceiverThreads(this, varargin)
            this.cppmethod('setNumReceiverThreads', varargin{:});
        end
        function time = getCurrentTime(this)
            time = this.cppmethod('getCurrentTime');
        end
        
        % if compiled with SMI integration only
        function startSMIDataSender(this)
            assert(this.hasSMIIntegration,'Can''t startSMIDataSender, UDPClient mex compiled without SMI integration')
            this.cppmethod('startSMIDataSender');
        end
        function removeSMIDataSender(this)
            assert(this.hasSMIIntegration,'Can''t removeSMIDataSender, UDPClient mex compiled without SMI integration')
            this.cppmethod('removeSMIDataSender');
        end
        
        % if compiled with Tobii integration only
        function connectToTobii(this,address)
            assert(this.hasTobiiIntegration,'Can''t connectToTobii, UDPClient mex compiled without Tobii integration')
            if isa(address,'string')
                address = char(address);    % seems matlab also has a string type, shows up if user accidentally uses double quotes, convert to char
            end
            this.cppmethod('connectToTobii', address);
        end
        function setTobiiScrSize(this,scrSize)
            assert(this.hasTobiiIntegration,'Can''t setTobiiScrSize, UDPClient mex compiled without Tobii integration')
            this.cppmethod('setTobiiScrSize', scrSize);
        end
        function setTobiiSampleRate(this,sampleFreq)
            assert(this.hasTobiiIntegration,'Can''t setTobiiSampleRate, UDPClient mex compiled without Tobii integration')
            this.cppmethod('setTobiiSampleRate', sampleFreq);
        end
        function startTobiiDataSender(this)
            assert(this.hasTobiiIntegration,'Can''t startTobiiDataSender, UDPClient mex compiled without Tobii integration')
            this.cppmethod('startTobiiDataSender');
        end
        function removeTobiiDataSender(this)
            assert(this.hasTobiiIntegration,'Can''t removeTobiiDataSender, UDPClient mex compiled without Tobii integration')
            this.cppmethod('removeTobiiDataSender');
        end
        
        
        % getters and setters of the matlab class itself
        function this = setComputerFilter(this, filter)
            % set default filter, as well as filter on C side.
            % NB: on the C-side, data acquired before this filter was set
            % is not filtered out and will be returned to the matlab code.
            % this means that if you call data getters that don't filter or
            % if you set an empty filter that allows anything through, you
            % may see a few of these samples. Calling filteredData getters
            % such as getDataGrouped with no input argument will filter out
            % these samples on the matlab side, so there is no issue
            % anymore
            this.computerFilter = filter;
            this.cppmethod('setComputerFilter', filter);
        end
        function setDebugLevel(this, debugLevel)
            % sets wanted debug output (each bit sets a different
            % functionality, add together to specify which you want):
            % 1: print received commands
            % 2: print sent commands
            % set to 0 to disable completely
            this.debugLevel = debugLevel;
        end
        function clearCommandBuffer(this)
            this.parsedCommandBuffer = {};
            this.getCommands(); % make sure we also clear the C++ buffer by requesting and discarding commands in there
        end
    end
    
    methods (Access = private, Hidden = true)
        function cmds = fillCommandBuffer(this)
            % get cmd messages from C++ buffer
            rawCmd = this.getCommands();
            % parse into {ip, timestamps, message}
            cmds   = cell(length(rawCmd),3);
            for p=1:length(rawCmd)
                if bitand(this.debugLevel,1)
                    fprintf('received from %d: %s\n',rawCmd(p).ip,rawCmd(p).text);
                end
                cmds{p,1} = rawCmd(p).ip;
                % moet los, anders krijg je doubles ookal zeg je %*f
                % eerst de timestamps
                commas= find(rawCmd(p).text==',');
                timestamps = sscanf(rawCmd(p).text(commas(end)+1:end),'%ld');
                % store
                cmds{p,2} = [timestamps rawCmd(p).timeStamp];   % send timestamp, receive timestamp
                cmds{p,3} = rawCmd(p).text(1:commas(end)-1);
            end
            % add parsed commands to buffer
            this.parsedCommandBuffer = [this.parsedCommandBuffer; cmds];
        end
    end
end
