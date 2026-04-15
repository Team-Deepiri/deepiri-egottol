library ieee;
use ieee.std_logic_1164.all;

entity jk_ff is
    port (
        j    : in  std_logic;
        k    : in  std_logic;
        clk  : in  std_logic;
        rst  : in  std_logic;
        q    : out std_logic;
        qn   : out std_logic
    );
end entity jk_ff;

architecture behavioral of jk_ff is
    signal q_reg : std_logic := '0';
begin
    process(clk, rst)
    begin
        if rst = '1' then
            q_reg <= '0';
        elsif rising_edge(clk) then
            if j = '0' and k = '0' then
                null;
            elsif j = '0' and k = '1' then
                q_reg <= '0';
            elsif j = '1' and k = '0' then
                q_reg <= '1';
            else
                q_reg <= not q_reg;
            end if;
        end if;
    end process;
    q  <= q_reg;
    qn <= not q_reg;
end architecture behavioral;
