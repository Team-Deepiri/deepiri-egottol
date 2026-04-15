library ieee;
use ieee.std_logic_1164.all;

entity xnor_gate is
    port (
        a : in  std_logic;
        b : in  std_logic;
        y : out std_logic
    );
end entity xnor_gate;

architecture behavioral of xnor_gate is
begin
    y <= not (a xor b);
end architecture behavioral;
