library ieee;
use ieee.std_logic_1164.all;

entity priority_encoder is
    port (
        a  : in  std_logic_vector(7 downto 0);
        y  : out std_logic_vector(2 downto 0);
        v  : out std_logic
    );
end entity priority_encoder;

architecture behavioral of priority_encoder is
begin
    process(a)
    begin
        v <= '0';
        y <= "000";
        
        if a(7) = '1' then
            y <= "111";
            v <= '1';
        elsif a(6) = '1' then
            y <= "110";
            v <= '1';
        elsif a(5) = '1' then
            y <= "101";
            v <= '1';
        elsif a(4) = '1' then
            y <= "100";
            v <= '1';
        elsif a(3) = '1' then
            y <= "011";
            v <= '1';
        elsif a(2) = '1' then
            y <= "010";
            v <= '1';
        elsif a(1) = '1' then
            y <= "001";
            v <= '1';
        elsif a(0) = '1' then
            y <= "000";
            v <= '1';
        end if;
    end process;
end architecture behavioral;
