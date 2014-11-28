require File.join(File.dirname(__FILE__), "..", "../lib/falapoa/parser.rb")

describe Parser do

  context "Parsing Cause Data" do 

    context "Causa com status Ativo" do
      before(:each) do
        File.open(File.join(File.dirname(__FILE__), "causes/ativo.html")) do |file|
          @html_body = Nokogiri::HTML file          
        end

        @valid_informations = {
          :status       => "Ativo",
          :proceeding   => "Aguardando informação",
          :destination  => "SMIC - Licenciamento de Atividades Localizadas",
          :last_update  => Date.strptime("2/3/2012", "%d/%m/%Y"),
          :service      => "Estabelecimento sem alvará",
          :due_date     => "2"
        }
      end

      it "deve retornar as informações da causa" do
        result = Parser.get_cause_data(@html_body, "0974611234")
        result[:data].should == @valid_informations
      end

      it "deve retornar o status correto" do
        result = Parser.get_status
        result.should == @valid_informations[:status]
      end

      it "deve retornar o trâmite correto" do
        result = Parser.get_proceeding
        result.should == @valid_informations[:proceeding]
      end
      
      it "deve retornar o destino da solicitação" do
        result = Parser.get_destination
        result.should == @valid_informations[:destination]
      end

      it "deve retornar a data da última atualização" do
        result = Parser.get_last_update
        result.should == @valid_informations[:last_update]
      end

      it "deve retornar o nome do serviço" do
        result = Parser.get_service
        result.should == @valid_informations[:service]
      end

      it "deve retornar os dias de prazo" do
        result = Parser.get_due_date
        result.should == @valid_informations[:due_date]
      end
    end

    context "Causa com status Cancelado" do
      before(:each) do
        File.open(File.join(File.dirname(__FILE__), "causes/cancelado.html")) do |file|
          @html_body = Nokogiri::HTML file          
        end

        @valid_informations = {
          :status       => "Cancelado",
          :proceeding   => "Análise",
          :destination  => "156 - Atendimento ao Cidadão",
          :last_update  => nil,
          :service      => "Formulário Internet",
          :due_date     => "1"
        }
      end

      it "deve retornar as informações da causa" do
        result = Parser.get_cause_data(@html_body, "0924581222")
        result[:data].should == @valid_informations
      end

      it "deve retornar o status correto" do
        result = Parser.get_status
        result.should == @valid_informations[:status]
      end

      it "deve retornar o trâmite correto" do
        result = Parser.get_proceeding
        result.should == @valid_informations[:proceeding]
      end

      it "deve retornar null como última atualização" do
        result = Parser.get_last_update
        result.should == @valid_informations[:last_update]
      end

      it "deve retornar null destino da solicitação" do
        result = Parser.get_destination
        result.should == @valid_informations[:destination]
      end

      it "deve retornar o nome do serviço" do
        result = Parser.get_service
        result.should == @valid_informations[:service]
      end

      it "deve retornar os dias de prazo" do
        result = Parser.get_due_date
        result.should == @valid_informations[:due_date]
      end
    end

    context "Causa com processo não enviado" do
      before(:each) do
        File.open(File.join(File.dirname(__FILE__), "causes/nao_enviado.html")) do |file|
          @html_body = Nokogiri::HTML file          
        end

        @valid_informations = {
          :status       => "Ativo",
          :proceeding   => "Processo não enviado para orgão executor!",
          :destination  => nil,
          :last_update  => nil,
          :service      => "Formulário Internet",
          :due_date     => nil
        }
      end

      it "deve retornar as informações da causa" do
        result = Parser.get_cause_data(@html_body, "1845221205")
        result[:data].should == @valid_informations
      end

      it "deve retornar o status correto" do
        result = Parser.get_status
        result.should == @valid_informations[:status]
      end

      it "deve retornar o trâmite correto" do
        result = Parser.get_proceeding
        result.should == @valid_informations[:proceeding]
      end

      it "deve retornar null como última atualização" do
        result = Parser.get_last_update
        result.should == @valid_informations[:last_update]
      end

      it "deve retornar null destino da solicitação" do
        result = Parser.get_destination
        result.should == @valid_informations[:destination]
      end

      it "deve retornar o nome do serviço" do
        result = Parser.get_service
        result.should == @valid_informations[:service]
      end

      it "deve retornar os dias de prazo" do
        result = Parser.get_due_date
        result.should == @valid_informations[:due_date]
      end
    end

    context "Causa com status Encerrado" do
      before(:each) do
        File.open(File.join(File.dirname(__FILE__), "causes/encerrado.html")) do |file|
          @html_body = Nokogiri::HTML file          
        end

        @valid_informations = {
          :status       => "Encerrado",
          :proceeding   => "Aguardando informação",
          :destination  => "SMIC - Licenciamento de Atividades Localizadas",
          :last_update  => Date.strptime("2/3/2012", "%d/%m/%Y"),
          :service      => "Estabelecimento sem alvará",
          :due_date     => "15"
        }
      end

      it "deve retornar as informações da causa" do
        result = Parser.get_cause_data(@html_body, "1140431137")
        result[:data].should == @valid_informations
      end

      it "deve retornar o status correto" do
        result = Parser.get_status
        result.should == @valid_informations[:status]
      end

      it "deve retornar o trâmite correto" do
        result = Parser.get_proceeding
        result.should == @valid_informations[:proceeding]
      end

      it "deve retornar null como última atualização" do
        result = Parser.get_last_update
        result.should == @valid_informations[:last_update]
      end

      it "deve retornar null destino da solicitação" do
        result = Parser.get_destination
        result.should == @valid_informations[:destination]
      end

      it "deve retornar o nome do serviço" do
        result = Parser.get_service
        result.should == @valid_informations[:service]
      end

      it "deve retornar os dias de prazo" do
        result = Parser.get_due_date
        result.should == @valid_informations[:due_date]
      end
    end
  end

  context "Parsing address" do

    before(:each) do

      @address_street = "Rua Moyses Antunes da Cunha"
      @address_no_district = "Av. Moyses Antunes da Cunha, 105 Porto Alegre RS"

      @unacceptable_addresses = [@address_street, @address_no_district]

      @address_cep = "90640-190"
      @address_no_number = "Rua Moyses Antunes da Cunha, Santo Antônio, Porto Alegre, Rio Grande do Sul, Brasil, 90640-190"
      @address_full = "R. Moyses Antunes da Cunha, 105 - Santo Antônio, Porto Alegre - RS, 90640-190, Brasil"
      @address_number_range = "R. Moyses Antunes da Cunha, 55-105 - Santo Antônio, Porto Alegre - Rio Grande do Sul, 90640-190, Brazil"
      @address_no_city = "Rua Moyses Antunes da Cunha, 105 cep:90640-190"

      @district = "Santo Antônio"
      @valid_cep = "90640-190"
      @valid_street = "Moyses Antunes da Cunha"
      @valid_number_1 = 105
      @valid_number_2 = 55
      @acceptable_addresses = [ @address_cep, @address_no_number, @address_full, @address_number_range, @address_no_city ]

      @all_addresses = @acceptable_addresses + @unacceptable_addresses
      @parser_proc = lambda {|a| Parser.get_address(a, @district) }
    end

    it "should return all the fixed data" do
      @all_addresses.each do |addr|
        result = @parser_proc.call(addr)
        result[:country].should == "Brasil"
        result[:state].should == "RS"
        result[:city].should == "Porto Alegre"
      end
    end

    context "accceptable address" do

      it "should return district Santo Antônio" do
        @acceptable_addresses.each do |addr|
          result = @parser_proc.call(addr)
          result[:district].should == @district
        end
      end

      it "should return CEP 90640-190 when available" do
        @acceptable_addresses.each do |addr|
          result = @parser_proc.call(addr)
          result[:cep].should == @valid_cep
        end
      end

      it "should return number 105 when single number is available" do
        [ @address_full, @address_no_city ].each do |addr|
          result = @parser_proc.call(addr)
          result[:number].should == @valid_number_1
        end
      end

      it "should return number 105 when there is a range" do
        result = @parser_proc.call(@address_number_range)
        result[:number].should == @valid_number_2
      end

      it "should return number 0 when there is no number available" do
        [ @address_cep, @address_no_number ].each do |addr|
          result = @parser_proc.call(addr)
          result[:number].should == 0
        end
      end

      it "should return street containing Moyses Antunes da Cunha" do
        addrs = @acceptable_addresses
        addrs.each do |addr|
          result = @parser_proc.call(addr)
          ret = result[:street].match(@valid_street)
          ret.should be_true
        end
      end
    end

    context "unacepptable address" do
      it "should return address for Secretaria Municipal de Administração de Porto Alegre" do
        @unacceptable_addresses.each do |addr|
          result = @parser_proc.call(addr)
          result[:cep].should == "90010-907"
          result[:district].should == "Centro"
          result[:street].should == "Rua Siqueira Campos"
          result[:number].should == 1300
        end
      end
    end
  end

  context "get_protocol" do

    context "there is a protocol successfully returned" do 
      before(:each) do
        soap_envelope = %{<?xml version="1.0" encoding="utf-8"?><soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema"><soap:Body><GravarSolicitacaoResponse xmlns="http://FormularioCC"><GravarSolicitacaoResult><GravaSolicitacao><codigoRetorno>1</codigoRetorno><NrSolicitacao>000392-12-57</NrSolicitacao><TramiteCriado>False</TramiteCriado></GravaSolicitacao></GravarSolicitacaoResult></GravarSolicitacaoResponse></soap:Body></soap:Envelope>}
        @result = Parser.get_protocol(soap_envelope)
      end

      it "should return correct protocol from soap envelope" do
        @result[:data].should == "0003921257"
      end

      it "should return correct protocol from soap envelope" do
        @result[:status].should == "success"
      end
    end

    context "there is an error" do 
      before(:each) do
        soap_envelope = %{<?xml version="1.0" encoding="utf-8"?><soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema"><soap:Body><GravarSolicitacaoResponse xmlns="http://FormularioCC"><GravarSolicitacaoResult><GravaSolicitacao><codigoRetorno>0</codigoRetorno><mensagem>ERROR_MESSAGE</mensagem><TramiteCriado>False</TramiteCriado></GravaSolicitacao></GravarSolicitacaoResult></GravarSolicitacaoResponse></soap:Body></soap:Envelope>}
        @result = Parser.get_protocol(soap_envelope)
      end

      it "should return correct protocol from soap envelope" do
        @result[:message].should == "ERROR_MESSAGE"
      end

      it "should return correct protocol from soap envelope" do
        @result[:status].should == "error"
      end
    end
  end
end