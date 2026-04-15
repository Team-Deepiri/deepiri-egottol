library ieee;
use ieee.std_logic_1164.all;

entity nand_gate is
    port (
        a : in  std_logic;
        b : in  std_logic;
        y : out std_logic
    );
end entity nand_gate;

architecture behavioral of nand_gate is
begin
    y <= not (a and b);
end architecture behavioral;
