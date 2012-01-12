/**
 * @author Iraklis Rossis
 */

/**
 * Returns an object that manages device sensor enumeration
 * @param {String} [type] The type of sensor to look for. Null for every sensor
 */
navigator.findSensors = function(type)
{
	return new SensorRequest(type);
};

/**
 * Initiate an enumeration request
 * @param {String} [type] The type of sensor to look for. Null for every sensor
 * @class This class handles sensor enumeration
 */
function SensorRequest(type)
{
	var self = this;

	this.result = [];
	this.readyState = "processing";
	if(type == undefined)
	{
		type = "";
	}
	this.type = type;
	var callbackId = "SensorManager" + PhoneGap.callbackId++;
    PhoneGap.callbacks[callbackId] = {success:
    									function(sensorList)
        								{
        									self.result = sensorList.result;
        									self.readyState = "done";
        									for(var i = 0; i < self.events.onsuccess.length; i++)
        									{
        										self.events.onsuccess[i](self.result);
        									}
        								}
        							};

	this.events = {
			/**
			 * Fires when the enumeration is completed
			 * @name SensorConnection#onsuccess
			 * @event
			 * @param {Object[]} results a list of the sensors
			 * @param {String} name the name of the sensor
			 * @param {String} type the type of the sensor
			 */
			"onsuccess": [],
	};

	/**
	 * Register a callback for an event
	 * @function
	 * @param {String} event the event to register
	 * @param {Function} listener the function that get called when the event fires
	 */
	this.addEventListener = function(event, listener, captureMethod)
	{
		if(self.events[event] != undefined)
		{
			self.events[event].push(listener);
		}
	};

	/**
	 * Unregister a callback
	 * @function
	 * @param {String} event the event to unregister
	 * @param {Function} listener the function that was registered
	 */
	this.removeEventListener = function(event, listener)
	{
		if(self.events[event] != undefined)
		{
			for(var i = 0; i < self.events[event].length; i++)
			{
				if(self.events[event] == listener)
				{
					self.events[event].splice(i,1);
					return;
				}
			}
		}
	};

	bridge.PhoneGap.send(callbackId, "SensorManager", "findSensors","{\"type\":\"" + type + "\"}");
}

/**
 * Sets up a connection to a sensor
 * @param {String|Object} options The sensor to connect to
 * @param {String} [options.name] The sensor to connect to
 * @param {String} [options.type] The sensor to connect to
 * @class This class manages a connection to a sensor
 */
function SensorConnection(options)
{
	var self = this;
	if(typeof options == "string")
	{
		this.type = options;
	}
	else if(typeof options.name == "string")
	{
		this.type = options.name;
	}
	else
	{
		this.type = options.type;
	}

	/**
	 * Starts receiving data from the sensor
	 * @function
	 * @param {Object} watchOptions
	 * @param {Number} interval time (in milliseconds) between sensor events
	 */
	this.startWatch = 	function(watchOptions)
						{
							if(self.status != "open")
							{
								var exception = new DOMException();
								exception.code = DOMException.INVALID_STATE_ERR;
								throw exception;
								return;
							}
							this.setStatus("watching");
							var callbackId = "SensorManager" + PhoneGap.callbackId++;
							PhoneGap.callbacks[callbackId] = {
									success:self.sensorEvent,
									fail:self.sensorError
							};
							bridge.PhoneGap.send(callbackId,
												"SensorManager",
												"startSensor",
												"{\"type\":\"" + self.type + "\", \"interval\":" + watchOptions.interval + "}");
						};
						/**
	 * Stops receiving data from the sensor
	 * @function
	 */
	this.endWatch = 	function()
						{
							if(self.status != "watching")
							{
								var exception = new DOMException();
								exception.code = DOMException.INVALID_STATE_ERR;
								throw exception;
								return;
							}
							this.setStatus("open");
							bridge.PhoneGap.send(null, "SensorManager", "stopSensor","{\"type\":\"" + self.type + "\"}");
						};
	/**
	 * Polls the sensor for a single reading
	 * @function
	*/
	this.read = 		function()
						{
							if(self.status != "open")
							{
								var exception = new DOMException();
								exception.code = DOMException.INVALID_STATE_ERR;
								throw exception;
								return;
							}
							var callbackId = "SensorManager" + PhoneGap.callbackId++;
							PhoneGap.callbacks[callbackId] = {
									success:self.sensorEvent
							};
							bridge.PhoneGap.send(callbackId,
									"SensorManager",
									"startSensor",
									"{\"type\":\"" + self.type + "\", \"interval\":-1}");
						};
	this.events = {
					/**
					 * Fires when the sensor sends new data
					 * @name SensorConnection#onsensordata
					 * @event
					 * @param {Object} sensorData data from the sensor
					 * @param {Object} sensorData.data specific metrics
					 * @param {Number} sensorData.data.x sensor value
					 * @param {Number} sensorData.data.y sensor value
					 * @param {Number} sensorData.data.z sensor value
					 * @param {Number} sensorData.timestamp time of sampling
					 * @param {String} sensorData.reason reason for sampling
					 */
					"onsensordata": [],
					/**
					 * Fires when there is an error
					 * @name SensorConnection#onerror
					 * @event
					 * @param {Object} error object describing the error
					 * @param {Number} error.code the error code
					 * @param {String} error.message the error message
					 */
					"onerror": [],
					/**
					 * Fires whenever the status of the connection changes
					 * @name SensorConnection#onstatuschange
					 * @event
					 * @param {String} status the new status of the connection
					 */
					"onstatuschange":[]
	};

	/**
	 * Register a callback for an event
	 * @function
	 * @param {String} event the event to register
	 * @param {Function} listener the function that get called when the event fires
	 */
	this.addEventListener = function(event, listener, captureMethod)
	{
		if(self.events[event] != undefined)
		{
			self.events[event].push(listener);
		}
	};

	/**
	 * Unregister a callback
	 * @function
	 * @param {String} event the event to unregister
	 * @param {Function} listener the function that was registered
	 */
	this.removeEventListener = function(event, listener)
	{
		if(self.events[event] != undefined)
		{
			for(var i = 0; i < self.events[event].length; i++)
			{
				if(self.events[event] == listener)
				{
					self.events[event].splice(i,1);
					return;
				}
			}
		}
	};

	this.sensorEvent = function(sensorData)
	{
		var event = {
						data: {
							x: sensorData.x,
							y: sensorData.y,
							z: sensorData.z
						},
						accuracy: "high",
						timestamp: sensorData.timestamp,
						reason:sensorData.reason
		};

		for(var i = 0; i < self.events.onsensordata.length; i++)
		{
			self.events.onsensordata[i](event);
		}
	};

	this.sensorError = function(errorData)
	{
		this.setStatus("error");
		var sensorError = {
							code: errorData.code,
							message: errorData.message
		};
		self.error = sensorError;
		for(var i = 0; i < self.events.onerror.length; i++)
		{
			self.events.onerror[i](sensorError);
		}
	};

	this.setStatus = function(status)
	{
		if(status != self.status)
		{
			self.status = status;
			for(var i = 0; i < self.events.onstatuschange.length; i++)
			{
				self.events.onstatuschange[i](status);
			}
		}
	};

	this.setStatus("open");
}