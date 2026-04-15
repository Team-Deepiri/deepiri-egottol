library ieee;
use ieee.std_logic_1164.all;

entity nor_gate is
    port (
        a : in  std_logic;
        b : in  std_logic;
        y : out std_logic
    );
end entity nor_gate;

architecture behavioral of nor_gate is
begin
    y <= not (a or b);
end architecture behavioral;
