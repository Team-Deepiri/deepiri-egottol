library ieee;
use ieee.std_logic_1164.all;

entity mux_2to1 is
    port (
        a : in  std_logic;
        b : in  std_logic;
        s : in  std_logic;
        y : out std_logic
    );
end entity mux_2to1;

architecture behavioral of mux_2to1 is
begin
    y <= a when s = '0' else b;
end architecture behavioral;
