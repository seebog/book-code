-- module3.lua
local instance

local M = function()
  if not instance then
    local var1 = 'ipsum'
    instance = {
      var2 = 'sit',
      lorem = function()
        return 'lorem'
      end,
      ipsum = function(self)
        return var1 .. self.var2
         end,
    }
  end
  return instance
end

return M
