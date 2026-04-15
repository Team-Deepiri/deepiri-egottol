library ieee;
use ieee.std_logic_1164.all;

entity xor_gate is
    port (
        a : in  std_logic;
        b : in  std_logic;
        y : out std_logic
    );
end entity xor_gate;

architecture behavioral of xor_gate is
begin
    y <= a xor b;
end architecture behavioral;
