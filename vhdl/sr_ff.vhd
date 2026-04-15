library ieee;
use ieee.std_logic_1164.all;

entity sr_ff is
    port (
        s    : in  std_logic;
        r    : in  std_logic;
        clk  : in  std_logic;
        rst  : in  std_logic;
        q    : out std_logic;
        qn   : out std_logic
    );
end entity sr_ff;

architecture behavioral of sr_ff is
    signal q_reg : std_logic := '0';
begin
    process(clk, rst)
    begin
        if rst = '1' then
            q_reg <= '0';
        elsif rising_edge(clk) then
            if s = '1' and r = '0' then
                q_reg <= '1';
            elsif s = '0' and r = '1' then
                q_reg <= '0';
            elsif s = '0' and r = '0' then
                null;
            else
                q_reg <= 'X';
            end if;
        end if;
    end process;
    q  <= q_reg;
    qn <= not q_reg;
end architecture behavioral;
