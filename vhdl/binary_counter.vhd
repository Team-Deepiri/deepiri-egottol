library ieee;
use ieee.std_logic_1164.all;

entity binary_counter is
    generic (
        N : positive := 8
    );
    port (
        clk  : in  std_logic;
        rst  : in  std_logic;
        load : in  std_logic;
        inc  : in  std_logic;
        d    : in  std_logic_vector(N-1 downto 0);
        q    : out std_logic_vector(N-1 downto 0)
    );
end entity binary_counter;

architecture behavioral of binary_counter is
    signal count : std_logic_vector(N-1 downto 0) := (others => '0');
begin
    process(clk, rst)
    begin
        if rst = '1' then
            count <= (others => '0');
        elsif rising_edge(clk) then
            if load = '1' then
                count <= d;
            elsif inc = '1' then
                count <= count + 1;
            end if;
        end if;
    end process;
    q <= count;
end architecture behavioral;
