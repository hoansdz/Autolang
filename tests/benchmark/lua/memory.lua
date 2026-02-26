local start = os.clock()

local function benchmarkMemory()
    local i = 0
    local list = nil

    while i < 1000000 do
        list = { data = i, next = list }
        i = i + 1
    end

    print("Created 1M nodes successfully")
end

benchmarkMemory()

print("Total time", os.clock() - start)