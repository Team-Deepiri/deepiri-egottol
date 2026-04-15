library ieee;
use ieee.std_logic_1164.all;

entity shift_register is
    generic (
        N : positive := 8
    );
    port (
        d    : in  std_logic;
        clk  : in  std_logic;
        rst  : in  std_logic;
        load : in  std_logic;
        q    : out std_logic_vector(N-1 downto 0)
    );
end entity shift_register;

architecture behavioral of shift_register is
    signal shift_reg : std_logic_vector(N-1 downto 0) := (others => '0');
begin
    process(clk, rst)
    begin
        if rst = '1' then
            shift_reg <= (others => '0');
        elsif rising_edge(clk) then
            if load = '1' then
                shift_reg <= d & shift_reg(N-1 downto 1);
            else
                shift_reg <= '0' & shift_reg(N-1 downto 1);
            end if;
        end if;
    end process;
    q <= shift_reg;
end architecture behavioral;
