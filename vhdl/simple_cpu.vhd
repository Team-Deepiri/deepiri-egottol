library ieee;
use ieee.std_logic_1164.all;

entity simple_cpu is
    generic (
        DATA_WIDTH : positive := 8;
        ADDR_WIDTH : positive := 8
    );
    port (
        clk      : in  std_logic;
        rst      : in  std_logic;
        data_in  : in  std_logic_vector(DATA_WIDTH-1 downto 0);
        data_out : out std_logic_vector(DATA_WIDTH-1 downto 0);
        address  : out std_logic_vector(ADDR_WIDTH-1 downto 0);
        mem_we   : out std_logic;
        mem_re   : out std_logic
    );
end entity simple_cpu;

architecture structural of simple_cpu is
    component cpu_datapath is
        generic (DATA_WIDTH, ADDR_WIDTH : positive);
        port (clk, rst, pc_ld, pc_inc, ir_ld, mar_ld, acc_ld, acc_inc, acc_dec : in std_logic;
              bus_sel : in std_logic_vector(1 downto 0); alu_sel : in std_logic_vector(2 downto 0);
              mem_we, mem_re : in std_logic; data_in : in std_logic_vector;
              data_out : out std_logic_vector; address : out std_logic_vector;
              zero, carry : out std_logic);
    end component;
    
    component control_unit is
        port (clk, rst : in std_logic; opcode : in std_logic_vector(3 downto 0); zero : in std_logic;
              pc_ld, pc_inc, ir_ld, mar_ld, acc_ld, acc_inc, acc_dec : out std_logic;
              bus_sel : out std_logic_vector(1 downto 0); alu_sel : out std_logic_vector(2 downto 0);
              mem_we, mem_re : out std_logic; halt : out std_logic);
    end component;
    
    signal pc_ld, pc_inc, ir_ld, mar_ld, acc_ld, acc_inc, acc_dec : std_logic;
    signal bus_sel : std_logic_vector(1 downto 0);
    signal alu_sel : std_logic_vector(2 downto 0);
    signal zero, carry, halt : std_logic;
    signal ir : std_logic_vector(DATA_WIDTH-1 downto 0);
begin
    DATAPATH: cpu_datapath generic map (DATA_WIDTH, ADDR_WIDTH)
        port map (clk, rst, pc_ld, pc_inc, ir_ld, mar_ld, acc_ld, acc_inc, acc_dec,
                   bus_sel, alu_sel, mem_we, mem_re, data_in, data_out, address, zero, carry);
    
    CONTROL: control_unit
        port map (clk, rst, ir(7 downto 4), zero, pc_ld, pc_inc, ir_ld, mar_ld,
                   acc_ld, acc_inc, acc_dec, bus_sel, alu_sel, mem_we, mem_re, halt);
    
    IR_PROC: process(clk)
    begin
        if rising_edge(clk) and ir_ld = '1' then
            ir <= data_in;
        end if;
    end process;
end architecture structural;
