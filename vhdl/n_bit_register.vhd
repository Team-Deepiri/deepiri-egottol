library ieee;
use ieee.std_logic_1164.all;

entity n_bit_register is
    generic (
        N : positive := 8
    );
    port (
        d    : in  std_logic_vector(N-1 downto 0);
        clk  : in  std_logic;
        rst  : in  std_logic;
        load : in  std_logic;
        q    : out std_logic_vector(N-1 downto 0)
    );
end entity n_bit_register;

architecture behavioral of n_bit_register is
begin
    process(clk, rst)
    begin
        if rst = '1' then
            q <= (others => '0');
        elsif rising_edge(clk) then
            if load = '1' then
                q <= d;
            end if;
        end if;
    end process;
end architecture behavioral;
