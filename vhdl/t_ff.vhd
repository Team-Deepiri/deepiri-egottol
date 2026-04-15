library ieee;
use ieee.std_logic_1164.all;

entity t_ff is
    port (
        t    : in  std_logic;
        clk  : in  std_logic;
        rst  : in  std_logic;
        q    : out std_logic;
        qn   : out std_logic
    );
end entity t_ff;

architecture behavioral of t_ff is
    signal q_reg : std_logic := '0';
begin
    process(clk, rst)
    begin
        if rst = '1' then
            q_reg <= '0';
        elsif rising_edge(clk) then
            if t = '1' then
                q_reg <= not q_reg;
            end if;
        end if;
    end process;
    q  <= q_reg;
    qn <= not q_reg;
end architecture behavioral;
