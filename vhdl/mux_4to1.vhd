library ieee;
use ieee.std_logic_1164.all;

entity mux_4to1 is
    port (
        a0 : in  std_logic;
        a1 : in  std_logic;
        a2 : in  std_logic;
        a3 : in  std_logic;
        s  : in  std_logic_vector(1 downto 0);
        y  : out std_logic
    );
end entity mux_4to1;

architecture behavioral of mux_4to1 is
begin
    with s select
        y <= a0 when "00",
             a1 when "01",
             a2 when "10",
             a3 when "11",
             'X' when others;
end architecture behavioral;
