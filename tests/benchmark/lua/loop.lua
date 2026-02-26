local start = os.clock()

local function test()
    local x = 0
    for i = 0, 100000000 - 1 do
        x = x + i
    end
    print(x)
end

test()

print("Total time", os.clock() - start)