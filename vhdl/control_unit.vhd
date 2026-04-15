library ieee;
use ieee.std_logic_1164.all;

entity control_unit is
    port (
        clk       : in  std_logic;
        rst       : in  std_logic;
        opcode    : in  std_logic_vector(3 downto 0);
        zero      : in  std_logic;
        pc_ld     : out std_logic;
        pc_inc    : out std_logic;
        ir_ld     : out std_logic;
        mar_ld    : out std_logic;
        acc_ld    : out std_logic;
        acc_inc   : out std_logic;
        acc_dec   : out std_logic;
        bus_sel   : out std_logic_vector(1 downto 0);
        alu_sel   : out std_logic_vector(2 downto 0);
        mem_we    : out std_logic;
        mem_re    : out std_logic;
        halt      : out std_logic
    );
end entity control_unit;

architecture behavioral of control_unit is
    type state_type is (FETCH, DECODE, EXECUTE);
    signal state, next_state : state_type;
begin
    process(clk, rst)
    begin
        if rst = '1' then
            state <= FETCH;
        elsif rising_edge(clk) then
            state <= next_state;
        end if;
    end process;
    
    process(state, opcode, zero)
    begin
        pc_ld <= '0'; pc_inc <= '0'; ir_ld <= '0'; mar_ld <= '0';
        acc_ld <= '0'; acc_inc <= '0'; acc_dec <= '0';
        bus_sel <= "00"; alu_sel <= "000";
        mem_we <= '0'; mem_re <= '0'; halt <= '0';
        
        case state is
            when FETCH =>
                pc_inc <= '1';
                mar_ld <= '1';
                bus_sel <= "10";
                mem_re <= '1';
                next_state <= DECODE;
            when DECODE =>
                ir_ld <= '1';
                next_state <= EXECUTE;
            when EXECUTE =>
                case opcode is
                    when "0000" =>
                        halt <= '1';
                    when "0001" =>
                        mar_ld <= '1';
                        bus_sel <= "00";
                        mem_re <= '1';
                        acc_ld <= '1';
                    when "0010" =>
                        mar_ld <= '1';
                        bus_sel <= "01";
                        mem_we <= '1';
                    when "0011" =>
                        acc_inc <= '1';
                    when "0100" =>
                        acc_dec <= '1';
                    when "0101" =>
                        pc_ld <= '1';
                        bus_sel <= "00";
                    when "0110" =>
                        if zero = '0' then
                            pc_inc <= '1';
                        end if;
                    when others =>
                        null;
                end case;
                next_state <= FETCH;
        end case;
    end process;
end architecture behavioral;
