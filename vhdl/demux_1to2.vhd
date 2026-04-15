library ieee;
use ieee.std_logic_1164.all;

entity demux_1to2 is
    port (
        a  : in  std_logic;
        s  : in  std_logic;
        y0 : out std_logic;
        y1 : out std_logic
    );
end entity demux_1to2;

architecture behavioral of demux_1to2 is
begin
    y0 <= a when s = '0' else '0';
    y1 <= a when s = '1' else '0';
end architecture behavioral;
