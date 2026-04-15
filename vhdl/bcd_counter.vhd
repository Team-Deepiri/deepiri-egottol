library ieee;
use ieee.std_logic_1164.all;

entity bcd_counter is
    port (
        clk  : in  std_logic;
        rst  : in  std_logic;
        inc  : in  std_logic;
        bcd  : out std_logic_vector(3 downto 0);
        cout : out std_logic
    );
end entity bcd_counter;

architecture behavioral of bcd_counter is
    signal count : std_logic_vector(3 downto 0) := (others => '0');
begin
    process(clk, rst)
    begin
        if rst = '1' then
            count <= (others => '0');
            cout  <= '0';
        elsif rising_edge(clk) then
            if inc = '1' then
                if count = "1001" then
                    count <= "0000";
                    cout  <= '1';
                else
                    count <= count + 1;
                    cout  <= '0';
                end if;
            end if;
        end if;
    end process;
    bcd <= count;
end architecture behavioral;
