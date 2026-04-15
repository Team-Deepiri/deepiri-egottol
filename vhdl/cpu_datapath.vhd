library ieee;
use ieee.std_logic_1164.all;

entity cpu_datapath is
    generic (
        DATA_WIDTH : positive := 8;
        ADDR_WIDTH : positive := 8
    );
    port (
        clk      : in  std_logic;
        rst      : in  std_logic;
        pc_ld    : in  std_logic;
        pc_inc   : in  std_logic;
        ir_ld    : in  std_logic;
        mar_ld   : in  std_logic;
        acc_ld   : in  std_logic;
        acc_inc  : in  std_logic;
        acc_dec  : in  std_logic;
        bus_sel  : in  std_logic_vector(1 downto 0);
        alu_sel  : in  std_logic_vector(2 downto 0);
        mem_we   : in  std_logic;
        mem_re   : in  std_logic;
        data_in  : in  std_logic_vector(DATA_WIDTH-1 downto 0);
        data_out : out std_logic_vector(DATA_WIDTH-1 downto 0);
        address  : out std_logic_vector(ADDR_WIDTH-1 downto 0);
        zero     : out std_logic;
        carry    : out std_logic
    );
end entity cpu_datapath;

architecture behavioral of cpu_datapath is
    component n_bit_alu is
        generic (N : positive := 8);
        port (
            a, b   : in  std_logic_vector(N-1 downto 0);
            op     : in  std_logic_vector(3 downto 0);
            cin    : in  std_logic;
            result : out std_logic_vector(N-1 downto 0);
            cout   : out std_logic;
            zero   : out std_logic
        );
    end component;
    
    component n_bit_register is
        generic (N : positive := 8);
        port (d, clk, rst, load : in  std_logic; q : out std_logic_vector(N-1 downto 0));
    end component;
    
    signal pc, mar, acc, ir, alu_out : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal alu_op : std_logic_vector(3 downto 0);
    signal bus_data : std_logic_vector(DATA_WIDTH-1 downto 0);
begin
    PC_REG: n_bit_register generic map (DATA_WIDTH) port map (bus_data, clk, rst, pc_ld, pc);
    MAR_REG: n_bit_register generic map (DATA_WIDTH) port map (bus_data, clk, rst, mar_ld, mar);
    ACC_REG: n_bit_register generic map (DATA_WIDTH) port map (bus_data, clk, rst, acc_ld, acc);
    IR_REG: n_bit_register generic map (DATA_WIDTH) port map (bus_data, clk, rst, ir_ld, ir);
    
    ALU: n_bit_alu generic map (DATA_WIDTH)
        port map (acc, bus_data, alu_op, '0', alu_out, open, zero);
    
    process(pc_inc, acc_inc, acc_dec)
    begin
        if pc_inc = '1' then
            pc <= std_logic_vector(unsigned(pc) + 1);
        end if;
        if acc_inc = '1' then
            acc <= std_logic_vector(unsigned(acc) + 1);
        end if;
        if acc_dec = '1' then
            acc <= std_logic_vector(unsigned(acc) - 1);
        end if;
    end process;
    
    with bus_sel select
        bus_data <= data_in when "00",
                    acc when "01",
                    pc when "10",
                    alu_out when "11",
                    (others => 'X') when others;
    
    data_out <= acc;
    address <= mar;
    carry <= '0';
end architecture behavioral;
