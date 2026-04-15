library ieee;
use ieee.std_logic_1164.all;

entity encoder_8to3 is
    port (
        a  : in  std_logic_vector(7 downto 0);
        y  : out std_logic_vector(2 downto 0)
    );
end entity encoder_8to3;

architecture behavioral of encoder_8to3 is
begin
    with a select
        y <= "000" when "00000001",
             "001" when "00000010",
             "010" when "00000100",
             "011" when "00001000",
             "100" when "00010000",
             "101" when "00100000",
             "110" when "01000000",
             "111" when "10000000",
             "XXX" when others;
end architecture behavioral;
