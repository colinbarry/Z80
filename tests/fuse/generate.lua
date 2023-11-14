if #arg < 3 then
  print("usage generate.lua <tests.in> <tests.expected> <output.c>")
  os.exit(1)
end

local infile = assert(io.open(arg[1], 'r'))
local expectedfile = assert(io.open(arg[2], 'r'))
local output = assert(io.open(arg[3], 'w'))

local function hex(s) return '0x' .. s end

-- converts the argument `str` to a string and wraps it in C-style 
-- "string quotes".
local function quote(arg) return '"' .. arg .. '"' end

-- Indents the given string by adding `n` leading indents to each line.
--
function indent(str, n)
  local n = n or 1
  local tabs = string.rep('    ', n)
  return tabs .. string.gsub(str, '\n', '\n' .. tabs)
end

-- Generates the text representation of a struct's member
-- 
local function struct(members)
  local t = {}
  for _, v in ipairs(members) do t[#t + 1] = '.' .. v[1] .. ' = ' .. v[2] end

  return table.concat(t, ',\n')
end

--  Converts the given data array to a C array string
local function data(d)
  local t = {}
  for _, v in ipairs(d) do t[#t + 1] = hex(v) end

  return '{' .. table.concat(t, ', ') .. '}'
end

-- Converts the registers state to a C struct
--
local function regstostring(regs)
  local t = {}
  for _, v in ipairs({
    'af', 'bc', 'de', 'hl', 'afp', 'bcp', 'dep', 'hlp', 'ix', 'iy', 'sp', 'pc',
    'i', 'r', 'iff1', 'iff2', 'im'
  }) do t[#t + 1] = {v, hex(regs[v])} end

  return '{\n' .. indent(struct(t)) .. '\n}'
end

-- Writes the given chunk as a C atruct
--
local function dumpchunk(chunk)
  return '{\n' .. indent(struct({
    {'addr', hex(chunk.addr)}, {'length', #chunk.bytes},
    {'data', data(chunk.bytes)}
  })) .. '\n}'
end

-- Writes the given chunks as a C array
--
local function dumpchunks(chunks)
  local buf = {}
  for _, v in ipairs(chunks) do buf[#buf + 1] = dumpchunk(v) end
  return '{\n' .. indent(table.concat(buf, ',\n')) .. '\n}'
end

-- Build a test from the input and expecation, and returns it as a C struct
-- string
--
local function assembletest(input, expectation)
  return '{\n' .. indent(struct({
    {'label', quote(input.label)}, {
      'assert',
      '{\n' .. indent(struct({{'regs', regstostring(expectation.regs)}})) ..
          '\n}'
    }, {
      'arrange', '{\n' .. indent(struct({
        {'regs', regstostring(input.regs)},
        {'halted', input.halted == '1' and 'true' or 'false'},
        {'cycles', input.cycles}, {'num_chunks', #input.chunks},
        {'chunks', dumpchunks(input.chunks)}
      })) .. '\n'
    }
  })) .. '}\n' .. '}'
end

-- Extract the information about a chunk
--
local function parsechunk(line)
  local bytes = {}
  local value
  local addr, pos = line:match('(%-?%x+)()')
  value, pos = line:match('(%-?%x+)()', pos)
  while value and value ~= '-1' do
    bytes[#bytes + 1] = value
    value, pos = line:match('(%-?%x+)()', pos)
  end

  return {addr = addr, bytes = bytes}
end

-- read registers, halted, and cycles
--
local function readcpustate(line1, line2)
  local inreg1match = '(%x%x%x%x) (%x%x%x%x) (%x%x%x%x) (%x%x%x%x) ' ..
                          '(%x%x%x%x) (%x%x%x%x) (%x%x%x%x) (%x%x%x%x) ' ..
                          '(%x%x%x%x) (%x%x%x%x) (%x%x%x%x)%s+(%x%x%x%x)'
  local inreg2match = '(%x%x) (%x%x) (%d) (%d) (%d) (%d)%s+(%d+)'
  local af, bc, de, hl, afp, bcp, dep, hlp, ix, iy, sp, pc = string.match(line1,
                                                                          inreg1match)
  local i, r, iff1, iff2, im, halted, cycles = string.match(line2, inreg2match)
  return {
    regs = {
      af = af,
      bc = bc,
      de = de,
      hl = hl,
      afp = afp,
      bcp = bcp,
      dep = dep,
      hlp = hlp,
      ix = ix,
      iy = iy,
      sp = sp,
      pc = pc,
      i = i,
      r = r,
      iff1 = iff1,
      iff2 = iff2,
      im = im
    },
    halted = halted,
    cycles = cycles
  }
end

-- Parses a single test from `infile`
--
local function readin()
  local label = infile:read('l')
  if label == nil then return end

  local l1 = infile:read('l')
  local l2 = infile:read('l')
  local cpustate = readcpustate(l1, l2)

  local chunks = {}
  local line = infile:read('l')
  while line ~= '-1' do
    chunks[#chunks + 1] = parsechunk(line)
    line = infile:read('l')
  end
  infile:read('l') -- skip blank line between tests

  return {
    label = label,
    regs = cpustate.regs,
    halted = cpustate.halted,
    cycles = cpustate.cycles,
    chunks = chunks
  }
end

-- Parses a single result from `expectedfile`
--
local function readexpectation()
  local function iseventline(line)
    for _, v in ipairs {'MR', 'MW', 'MC', 'PR', 'PW', 'PC'} do
      if line:find(v) then return true end
    end
  end

  local label = expectedfile:read('l')
  if label == nil then return end

  local line = expectedfile:read('l')
  while iseventline(line) do line = expectedfile:read('l') end

  local cpustate = readcpustate(line, expectedfile:read('l'))

  -- skip the trailing memory lines
  line = expectedfile:read('l')
  while line and #line > 0 do line = expectedfile:read('l') end

  return {
    label = label,
    regs = cpustate.regs,
    halted = cpustate.halted,
    cycles = cpustate.cycles
  }
end

local tests = {}
local test = readin()
while test do
  tests[#tests + 1] = test
  test = readin()
end

local expectations = {}
local expectation = readexpectation()
while expectation do
  expectations[expectation.label] = expectation
  expectation = readexpectation()
end

for i = 1, #tests do
  local expectation = assert(expectations[tests[i].label],
                             "expectation not found")
  tests[i] = assembletest(tests[i], expectation)
end

local template = [[
/* generated fuse tests  */
#include "fuse-tests.h"

struct Tests tests = {
]] .. indent(struct {
  {'num_tests', #tests},
  {'tests', '{\n' .. indent(table.concat(tests, ',\n')) .. '\n}'}
}) .. '\n};\n'

output:write(template)
output:close()
