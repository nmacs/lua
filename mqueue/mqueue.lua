local base = G_
local coroutine = require("coroutine")
local scheduler = require("scheduler")
local table     = require("table")

module("mqueue")

local received  = {}
local receiving = {}

local function push_message(thread, msg)
	local arr = received[thread]
	if not arr then
		received[thread] = {msg}
	else
		table.insert(arr, 1, msg)
	end
end

local function pop_message(thread)
	local arr = received[thread]
	if not arr then
		return nil
	end
	return table.remove(arr)	
end

function send(thread, msg)
	push_message(thread, msg)
	if receiving[thread] then
		scheduler.cancel_wait(thread)
	end
end

function multicast(threads, msg)
	for _, thread in base.ipairs(threads) do
		send(thread, msg)
		scheduler.msleep(0) -- yield
	end
end

function receive(timeout)
	local current = coroutine.running()
	local msg = pop_message(current)
	if msg then return msg end
	receiving[current] = true
	scheduler.msleep(timeout)
	receiving[current] = false
	return pop_message(current)
end