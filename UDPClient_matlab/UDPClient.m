% MATLAB class wrapper to underlying C++ class UDPClient

classdef UDPClient < handle
    properties (Access = private, Hidden = true)
        objectHandle; % Handle to the underlying C++ class instance
        computerFilter = [];
        hasSMIIntegration;
    end
    methods
        %% Constructor - Create a new C++ class instance 
        function this = UDPClient(varargin)
            this.objectHandle       = UDPClient_matlab('new', varargin{:});
            this.hasSMIIntegration  = UDPClient_matlab('hasSMIIntegration', this.objectHandle);
        end
        
        %% Destructor - Destroy the C++ class instance
        function delete(this)
            UDPClient_matlab('deInit', this.objectHandle);
            UDPClient_matlab('delete', this.objectHandle);
        end

        %% methods
        function init(this)
            UDPClient_matlab('init', this.objectHandle);
        end
        function deInit(this)
            UDPClient_matlab('deInit', this.objectHandle);
        end
        function sendWithTimeStamp(this, varargin)
            UDPClient_matlab('sendWithTimeStamp', this.objectHandle, varargin{:});
        end
        function send(this, varargin)
            UDPClient_matlab('send', this.objectHandle, varargin{:});
        end
        % check if receiver threads are still running. They may close when
        % an exit message is received. function returns how many threads
        % are running
        function numRunning = checkReceiverThreads(this)
            numRunning = UDPClient_matlab('checkReceiverThreads', this.objectHandle);
        end
        
        % get the data and command messages received since the last call to this function
        function data = getData(this)
            data = UDPClient_matlab('getData', this.objectHandle);
        end
        function cmds = getCommands(this)
            cmds = UDPClient_matlab('getCommands', this.objectHandle);
        end
        % get data grouped by computer
        function data = getDataGrouped(this,computers)
            if nargin<2
                computers = this.computerFilter;
            end
            rawData = this.getData();
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
        % get cmds, filtered and parsed
        function cmds = getCommandsFiltered(this,computers)
            if nargin<2
                computers = this.computerFilter;
            end
            rawCmd = this.getCommands();
            cmds = cell(length(rawCmd),3);
            for p=1:length(rawCmd)
                % optionally, only process messages from specified
                % computers
                if ~isempty(computers) && ~any(rawCmd(p).ip==computers)
                    continue
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
            % remove empty lines (can happen if we skip a command)
            cmds(cellfun(@isempty,cmds(:,1)),:) = [];
        end
        
        % getters and setters
        function setUseWTP(this, varargin)
            UDPClient_matlab('setUseWTP', this.objectHandle, varargin{:});
        end
        function setMaxClockRes(this, varargin)
            UDPClient_matlab('setMaxClockRes', this.objectHandle, varargin{:});
        end
        function loopback = getLoopBack(this)
            loopback = UDPClient_matlab('getLoopBack', this.objectHandle);
        end
        function setLoopBack(this, varargin)
            UDPClient_matlab('setLoopBack', this.objectHandle, varargin{:});
        end
        function groupAddress = getGroupAddress(this)
            groupAddress = UDPClient_matlab('getGroupAddress', this.objectHandle);
        end
        function setGroupAddress(this, varargin)
            UDPClient_matlab('setGroupAddress', this.objectHandle, varargin{:});
        end
        function port = getPort(this)
            port = UDPClient_matlab('getPort', this.objectHandle);
        end
        function setPort(this, varargin)
            UDPClient_matlab('setPort', this.objectHandle, varargin{:});
        end
        function bufferSize = getBufferSize(this)
            bufferSize = UDPClient_matlab('getBufferSize', this.objectHandle);
        end
        function setBufferSize(this, varargin)
            UDPClient_matlab('setBufferSize', this.objectHandle, varargin{:});
        end
        function numQueuedReceives = getNumQueuedReceives(this)
            numQueuedReceives = UDPClient_matlab('getNumQueuedReceives', this.objectHandle);
        end
        function setNumQueuedReceives(this, varargin)
            UDPClient_matlab('setNumQueuedReceives', this.objectHandle, varargin{:});
        end
        function numReceiverThreads = getNumReceiverThreads(this)
            numReceiverThreads = UDPClient_matlab('getNumReceiverThreads', this.objectHandle);
        end
        function setNumReceiverThreads(this, varargin)
            UDPClient_matlab('setNumReceiverThreads', this.objectHandle, varargin{:});
        end
        function time = getCurrentTime(this)
            time = UDPClient_matlab('getCurrentTime', this.objectHandle);
        end
        
        % if compiled with SMI integration only
        function startSMIDataSender(this)
            assert(this.hasSMIIntegration,'Can''t startSMIDataSender, UDPClient mex compiled without SMI integration')
            UDPClient_matlab('startSMIDataSender', this.objectHandle);
        end
        function removeSMIDataSender(this)
            assert(this.hasSMIIntegration,'Can''t startSMIDataSender, UDPClient mex compiled without SMI integration')
            UDPClient_matlab('removeSMIDataSender', this.objectHandle);
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
            UDPClient_matlab('setComputerFilter', this.objectHandle, filter);
        end
    end
end
