library ieee;
use ieee.std_logic_1164.all;

entity ripple_carry_adder is
    generic (
        N : positive := 8
    );
    port (
        a    : in  std_logic_vector(N-1 downto 0);
        b    : in  std_logic_vector(N-1 downto 0);
        cin  : in  std_logic;
        sum  : out std_logic_vector(N-1 downto 0);
        cout : out std_logic
    );
end entity ripple_carry_adder;

architecture behavioral of ripple_carry_adder is
    signal c : std_logic_vector(N downto 0);
begin
    c(0) <= cin;
    
    gen_adder : for i in 0 to N-1 generate
        sum(i) <= a(i) xor b(i) xor c(i);
        c(i+1) <= (a(i) and b(i)) or (c(i) and (a(i) xor b(i)));
    end generate;
    
    cout <= c(N);
end architecture behavioral;
