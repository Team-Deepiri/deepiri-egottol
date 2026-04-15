library ieee;
use ieee.std_logic_1164.all;

entity comparator is
    generic (
        N : positive := 8
    );
    port (
        a   : in  std_logic_vector(N-1 downto 0);
        b   : in  std_logic_vector(N-1 downto 0);
        lt  : out std_logic;
        eq  : out std_logic;
        gt  : out std_logic
    );
end entity comparator;

architecture behavioral of comparator is
begin
    process(a, b)
    begin
        if a < b then
            lt <= '1';
            eq <= '0';
            gt <= '0';
        elsif a = b then
            lt <= '0';
            eq <= '1';
            gt <= '0';
        else
            lt <= '0';
            eq <= '0';
            gt <= '1';
        end if;
    end process;
end architecture behavioral;
