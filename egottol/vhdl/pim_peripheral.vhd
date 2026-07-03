-- AXI4-Lite PIM (processing-in-memory) peripheral stub for crossbar weight programming.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity pim_peripheral is
    generic (
        C_S_AXI_DATA_WIDTH : integer := 32;
        C_S_AXI_ADDR_WIDTH : integer := 8
    );
    port (
        S_AXI_ACLK    : in  std_logic;
        S_AXI_ARESETN : in  std_logic;

        S_AXI_AWADDR  : in  std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
        S_AXI_AWVALID : in  std_logic;
        S_AXI_AWREADY : out std_logic;

        S_AXI_WDATA   : in  std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
        S_AXI_WSTRB   : in  std_logic_vector((C_S_AXI_DATA_WIDTH/8)-1 downto 0);
        S_AXI_WVALID  : in  std_logic;
        S_AXI_WREADY  : out std_logic;

        S_AXI_BRESP   : out std_logic_vector(1 downto 0);
        S_AXI_BVALID  : out std_logic;
        S_AXI_BREADY  : in  std_logic;

        S_AXI_ARADDR  : in  std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
        S_AXI_ARVALID : in  std_logic;
        S_AXI_ARREADY : out std_logic;

        S_AXI_RDATA   : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
        S_AXI_RRESP   : out std_logic_vector(1 downto 0);
        S_AXI_RVALID  : out std_logic;
        S_AXI_RREADY  : in  std_logic
    );
end entity pim_peripheral;

architecture rtl of pim_peripheral is
    constant REG_WEIGHT_ADDR : unsigned(C_S_AXI_ADDR_WIDTH-1 downto 0) := x"00";
    constant REG_WEIGHT_DATA : unsigned(C_S_AXI_ADDR_WIDTH-1 downto 0) := x"04";
    constant REG_INPUT_VEC   : unsigned(C_S_AXI_ADDR_WIDTH-1 downto 0) := x"08";
    constant REG_OUTPUT_VEC  : unsigned(C_S_AXI_ADDR_WIDTH-1 downto 0) := x"0C";

    signal weight_addr_reg : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
    signal weight_data_reg : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
    signal input_vec_reg   : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
    signal output_vec_reg  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);

    signal awready_i : std_logic := '0';
    signal wready_i  : std_logic := '0';
    signal bvalid_i  : std_logic := '0';
    signal arready_i : std_logic := '0';
    signal rvalid_i  : std_logic := '0';
    signal rdata_i   : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0) := (others => '0');

    signal write_addr : unsigned(C_S_AXI_ADDR_WIDTH-1 downto 0);
    signal read_addr  : unsigned(C_S_AXI_ADDR_WIDTH-1 downto 0);
begin
    S_AXI_AWREADY <= awready_i;
    S_AXI_WREADY  <= wready_i;
    S_AXI_BRESP   <= "00";
    S_AXI_BVALID  <= bvalid_i;
    S_AXI_ARREADY <= arready_i;
    S_AXI_RDATA   <= rdata_i;
    S_AXI_RRESP   <= "00";
    S_AXI_RVALID  <= rvalid_i;

    process(S_AXI_ACLK)
    begin
        if rising_edge(S_AXI_ACLK) then
            if S_AXI_ARESETN = '0' then
                weight_addr_reg <= (others => '0');
                weight_data_reg <= (others => '0');
                input_vec_reg   <= (others => '0');
                output_vec_reg  <= (others => '0');
                awready_i       <= '0';
                wready_i        <= '0';
                bvalid_i        <= '0';
                arready_i       <= '0';
                rvalid_i        <= '0';
                rdata_i         <= (others => '0');
            else
                awready_i <= S_AXI_AWVALID and not awready_i;
                wready_i  <= S_AXI_WVALID and awready_i;

                if S_AXI_WVALID = '1' and wready_i = '1' then
                    write_addr <= unsigned(S_AXI_AWADDR);
                    case unsigned(S_AXI_AWADDR) is
                        when REG_WEIGHT_ADDR =>
                            weight_addr_reg <= S_AXI_WDATA;
                        when REG_WEIGHT_DATA =>
                            weight_data_reg <= S_AXI_WDATA;
                        when REG_INPUT_VEC =>
                            input_vec_reg <= S_AXI_WDATA;
                        when others =>
                            null;
                    end case;
                    bvalid_i <= '1';
                elsif S_AXI_BREADY = '1' then
                    bvalid_i <= '0';
                end if;

                arready_i <= S_AXI_ARVALID and not arready_i;

                if S_AXI_ARVALID = '1' and arready_i = '1' then
                    read_addr <= unsigned(S_AXI_ARADDR);
                    case unsigned(S_AXI_ARADDR) is
                        when REG_WEIGHT_ADDR =>
                            rdata_i <= weight_addr_reg;
                        when REG_WEIGHT_DATA =>
                            rdata_i <= weight_data_reg;
                        when REG_INPUT_VEC =>
                            rdata_i <= input_vec_reg;
                        when REG_OUTPUT_VEC =>
                            rdata_i <= output_vec_reg;
                        when others =>
                            rdata_i <= (others => '0');
                    end case;
                    rvalid_i <= '1';
                elsif S_AXI_RREADY = '1' then
                    rvalid_i <= '0';
                end if;

                -- Simple dot-product placeholder: latch input bits into output.
                output_vec_reg <= std_logic_vector(
                    unsigned(output_vec_reg) xor unsigned(input_vec_reg xor weight_data_reg)
                );
            end if;
        end if;
    end process;
end architecture rtl;
