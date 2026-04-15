library ieee;
use ieee.std_logic_1164.all;

entity d_ff is
    port (
        d    : in  std_logic;
        clk  : in  std_logic;
        rst  : in  std_logic;
        q    : out std_logic;
        qn   : out std_logic
    );
end entity d_ff;

architecture behavioral of d_ff is
begin
    process(clk, rst)
    begin
        if rst = '1' then
            q  <= '0';
            qn <= '1';
        elsif rising_edge(clk) then
            q  <= d;
            qn <= not d;
        end if;
    end process;
end architecture behavioral;
