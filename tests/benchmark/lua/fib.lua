local start = os.clock()

local function fib(n)
    if n < 2 then
        return n
    end
    return fib(n - 1) + fib(n - 2)
end

local function benchmarkFib()
    local n = 35
    local result = fib(n)
    print("Fibonacci(35) = " .. result)
end

benchmarkFib()
print("Total time", os.clock() - start)