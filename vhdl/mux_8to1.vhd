library ieee;
use ieee.std_logic_1164.all;

entity mux_8to1 is
    port (
        a0 : in  std_logic;
        a1 : in  std_logic;
        a2 : in  std_logic;
        a3 : in  std_logic;
        a4 : in  std_logic;
        a5 : in  std_logic;
        a6 : in  std_logic;
        a7 : in  std_logic;
        s  : in  std_logic_vector(2 downto 0);
        y  : out std_logic
    );
end entity mux_8to1;

architecture behavioral of mux_8to1 is
begin
    with s select
        y <= a0 when "000",
             a1 when "001",
             a2 when "010",
             a3 when "011",
             a4 when "100",
             a5 when "101",
             a6 when "110",
             a7 when "111",
             'X' when others;
end architecture behavioral;
