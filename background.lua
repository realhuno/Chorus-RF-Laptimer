local INTERVAL          = 50         -- in 1/100th seconds
local MSP_SET_RTC       = 246
local MSP_TX_INFO       = 186

local lastRunTS
local sensorId = -1
local timeIsSet = false
local mspMsgQueued = false

local function getSensorValue()
    if sensorId == -1 then
        local sensor = getFieldInfo(protocol.stateSensor)
        if type(sensor) == "table" then
            sensorId = sensor['id'] or -1
        end
    end
    return getValue(sensorId)
end

local function modelActive(sensorValue)
    return type(sensorValue) == "number" and sensorValue > 0
end

local function init()
    lastRunTS = 0
end

local function run_bg()
    -- run in intervals
	local valsTemp = {}
	local time1=getValue('1AEB')
	-- local time2=getValue('trn6')

	
	if string.byte(time1,1) == nil then valsTemp[1]=32 else valsTemp[1]=string.byte(time1,1) end
    if string.byte(time1,2) == nil then valsTemp[2]=32 else valsTemp[2]=string.byte(time1,2) end
	if string.byte(time1,3) == nil then valsTemp[3]=32 else valsTemp[3]=string.byte(time1,3) end
	if string.byte(time1,4) == nil then valsTemp[4]=32 else valsTemp[4]=string.byte(time1,4) end
	if string.byte(time1,5) == nil then valsTemp[5]=32 else valsTemp[5]=string.byte(time1,5) end
	if string.byte(time1,6) == nil then valsTemp[6]=32 else valsTemp[6]=string.byte(time1,6) end
	if string.byte(time1,7) == nil then valsTemp[7]=32 else valsTemp[7]=string.byte(time1,7) end
	if string.byte(time1,8) == nil then valsTemp[8]=32 else valsTemp[8]=string.byte(time1,8) end
	
	-- valsTemp[5]=32
	-- if string.byte(time2,1) == nil then valsTemp[6]=32 else valsTemp[6]=string.byte(time2,1) end
	-- if string.byte(time2,2) == nil then valsTemp[7]=32 else valsTemp[7]=string.byte(time2,2) end
	-- if string.byte(time2,3) == nil then valsTemp[8]=32 else valsTemp[8]=string.byte(time2,3) end
	-- if string.byte(time2,4) == nil then valsTemp[9]=32 else valsTemp[9]=string.byte(time2,4) end

	protocol.mspWrite(11, valsTemp)
		
    if lastRunTS == 0 or lastRunTS + INTERVAL < getTime() then
        mspMsgQueued = false
        -- ------------------------------------
        -- SYNC DATE AND TIME
        -- ------------------------------------
        local sensorValue = getSensorValue()

        if not timeIsSet and modelActive(sensorValue) then
            -- Send datetime when the telemetry connection is available
            -- assuming when sensor value higher than 0 there is an telemetry connection
            -- only send datetime one time after telemetry connection became available
            -- or when connection is restored after e.g. lipo refresh
            local now = getDateTime()
            local year = now.year;

            values = {}
            values[1] = bit32.band(year, 0xFF)
            year = bit32.rshift(year, 8)
            values[2] = bit32.band(year, 0xFF)
            values[3] = now.mon
            values[4] = now.day
            values[5] = now.hour
            values[6] = now.min
            values[7] = now.sec

            -- send msp message
            protocol.mspWrite(MSP_SET_RTC, values)
            mspMsgQueued = true

            timeIsSet = true
        elseif not modelActive(sensorValue) then
            timeIsSet = false
        end


        -- ------------------------------------
        -- SEND RSSI VALUE
        -- ------------------------------------

        if mspMsgQueued == false then
            local rssi, alarm_low, alarm_crit = getRSSI()
            -- Scale the [0, 85] (empirical) RSSI values to [0, 255]
            rssi = rssi * 3
            if rssi > 255 then
                rssi = 255
            end

            values = {}
            values[1] = rssi

            -- send msp message
            protocol.mspWrite(MSP_TX_INFO, values)
            mspMsgQueued = true
        end

        lastRunTS = getTime()
    end

    -- process queue
    mspProcessTxQ()

end

return { init=init, run_bg=run_bg }
