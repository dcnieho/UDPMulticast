% MATLAB class wrapper to underlying C++ class UDPClient

classdef UDPClient < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle; % Handle to the underlying C++ class instance
    end
    methods
        %% Constructor - Create a new C++ class instance 
        function this = UDPClient(varargin)
            this.objectHandle = UDPClient_matlab('new', varargin{:});
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
        
        % getters and setters
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
    end
end
