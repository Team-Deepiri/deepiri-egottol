library ieee;
use ieee.std_logic_1164.all;

entity carry_lookahead_adder is
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
end entity carry_lookahead_adder;

architecture behavioral of carry_lookahead_adder is
    component half_adder is
        port (
            a    : in  std_logic;
            b    : in  std_logic;
            sum  : out std_logic;
            cout : out std_logic
        );
    end component;
    
    signal p, g : std_logic_vector(N-1 downto 0);
    signal c    : std_logic_vector(N downto 0);
begin
    c(0) <= cin;
    
    gen_cla : for i in 0 to N-1 generate
        p(i) <= a(i) xor b(i);
        g(i) <= a(i) and b(i);
        
        sum(i) <= p(i) xor c(i);
        
        c(i+1) <= g(i) or (p(i) and c(i));
    end generate;
    
    cout <= c(N);
end architecture behavioral;
