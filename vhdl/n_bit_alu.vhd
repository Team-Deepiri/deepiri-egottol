library ieee;
use ieee.std_logic_1164.all;

entity n_bit_alu is
    generic (
        N : positive := 8
    );
    port (
        a    : in  std_logic_vector(N-1 downto 0);
        b    : in  std_logic_vector(N-1 downto 0);
        op   : in  std_logic_vector(3 downto 0);
        cin  : in  std_logic;
        result : out std_logic_vector(N-1 downto 0);
        cout : out std_logic;
        zero : out std_logic
    );
end entity n_bit_alu;

architecture behavioral of n_bit_alu is
    signal sum : std_logic_vector(N downto 0);
begin
    process(a, b, op, cin)
    begin
        sum <= (others => '0');
        cout <= '0';
        
        case op is
            when "0000" =>
                sum <= ('0' & a) + ('0' & b);
                result <= sum(N-1 downto 0);
                cout <= sum(N);
            when "0001" =>
                sum <= ('0' & a) - ('0' & b);
                result <= sum(N-1 downto 0);
                cout <= sum(N);
            when "0010" =>
                result <= a and b;
            when "0011" =>
                result <= a or b;
            when "0100" =>
                result <= a xor b;
            when "0101" =>
                result <= a nand b;
            when "0110" =>
                result <= a nor b;
            when "0111" =>
                result <= not a;
            when "1000" =>
                sum <= ('0' & a) + ('0' & b) + (cin & (N-1 downto 0 => '0'));
                result <= sum(N-1 downto 0);
                cout <= sum(N);
            when others =>
                result <= (others => '0');
        end case;
    end process;
    
    zero <= '1' when result = (result'range => '0') else '0';
end architecture behavioral;
