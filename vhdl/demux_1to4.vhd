library ieee;
use ieee.std_logic_1164.all;

entity demux_1to4 is
    port (
        a  : in  std_logic;
        s  : in  std_logic_vector(1 downto 0);
        y0 : out std_logic;
        y1 : out std_logic;
        y2 : out std_logic;
        y3 : out std_logic
    );
end entity demux_1to4;

architecture behavioral of demux_1to4 is
begin
    y0 <= a when s = "00" else '0';
    y1 <= a when s = "01" else '0';
    y2 <= a when s = "10" else '0';
    y3 <= a when s = "11" else '0';
end architecture behavioral;
