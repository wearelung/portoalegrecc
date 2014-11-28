require File.join(File.dirname(__FILE__), "..", "../lib/falapoa/bootstrap.rb")

class Parser

  def self.get_address_body(cause, user)
    addr = Parser.get_address(cause[:local], cause[:district])
    request_body = Hash.new
    clean_abstract = cause[:abstract].gsub(/<\/?[^>]*>/, "")
    # request_body["ins0:Servico"] = FALAPOA_CONFIG[:service]
    # request_body["ins0:Servico"] = 72385 # ABL
    request_body["ins0:Servico"] = 73083 # PROCEMPA
    request_body["ins0:Assunto"] = "(Url no Portoalegre.cc: #{cause[:url]}) - #{clean_abstract}"
    request_body["ins0:Nome"] = user[:name]
    request_body["ins0:email"] = user[:email]
    request_body["ins0:ddd"] = ""
    request_body["ins0:Telefone"] = ""
    request_body["ins0:dddCel"] = ""
    request_body["ins0:NumCel"] = ""
    request_body["ins0:dddCom"] = ""
    request_body["ins0:TelCom"] = ""
    request_body["ins0:TelContato"] = ""
    request_body["ins0:Logradouro"] = addr[:street]
    request_body["ins0:Numero"] = addr[:number].to_i
    request_body["ins0:ComplementoLogradouro"] = ""
    request_body["ins0:Bairro"] = addr[:district]
    request_body["ins0:CEP"] = addr[:cep].gsub(/-/, "")
    request_body["ins0:Cidade"] = "Porto Alegre"
    request_body["ins0:Estado"] = "RS"
    request_body[:order!] = ["ins0:Servico", "ins0:Assunto", "ins0:Nome", "ins0:email", "ins0:ddd", 
      "ins0:Telefone", "ins0:dddCel", "ins0:NumCel", "ins0:dddCom", "ins0:TelCom", "ins0:TelContato", 
      "ins0:Logradouro", "ins0:Numero", "ins0:ComplementoLogradouro", "ins0:Bairro", "ins0:CEP", "ins0:Cidade", "ins0:Estado"]
    request_body
  end

  def self.get_protocol(response)
    response_doc = Nokogiri::XML response.to_s
    return_code = response_doc.at_xpath("//bd:codigoRetorno", "bd" => %{http://FormularioCC}).content
    if return_code == "1"
      returning_protocol = response_doc.at_xpath("//bd:NrSolicitacao", "bd" => %{http://FormularioCC}).content
      { :status => "success", :data => returning_protocol.gsub!(/\D/, "") }
    else 
      returning_message = response_doc.at_xpath("//bd:mensagem", "bd" => %{http://FormularioCC}).content
      { :status => "error", :message => returning_message }
    end
  end

  def self.get_address(address, district)
    @changeable_address = address.clone
    @addr = { :country => "Brasil", :state => "RS", :city => "Porto Alegre" }

    remove_static_data(district)
    
    cep = /\d{5}-\d{3}/.match(@changeable_address).to_s

    if cep.blank?
      @addr[:cep] = "90010-907"
      @addr[:district] = "Centro"
      @addr[:street] = "Rua Siqueira Campos"
      @addr[:number] = 1300
    else
      @addr[:district] = district
      @changeable_address.gsub!(cep, "")
      @addr[:cep] = cep
      street_and_number = @changeable_address.split(",")
      
      if cep == @changeable_address || street_and_number.empty?
        # Buscar Logradouro pelo CEP
        form_data = {:cep => cep}
        xml_doc = Nokogiri::XML RestClient.post("http://cep.republicavirtual.com.br/web_cep.php", form_data)
        @addr[:district] = xml_doc.css("bairro").text
        @addr[:street] = xml_doc.css("logradouro").text
        @addr[:number] = 0
      else
        @addr[:street] = street_and_number[0]
        if street_and_number.size > 1
          @addr[:number] = street_and_number[1].gsub(/[^0-9\-]+/, "").to_i
        else
          @addr[:number] = 0
        end
      end
    end
    @addr
  end

  def self.get_cause_data(html, prot)
    @html_doc = Nokogiri::HTML html.to_s
    if @html_doc.at_css("#ctl00_cphDefault_tblPesquisa")
      { :status => "error", :message => "Protocolo em análise", :protocol => prot }
    else
      @node = @html_doc.css("#ctl00_cphDefault_dgrTramite tr").last
      { :status => "success",
        :data => ret = {
          :status       => get_status,
          :proceeding   => get_proceeding,
          :destination  => get_destination,
          :last_update  => get_last_update,
          :service      => get_service,
          :due_date     => get_due_date
        }
      }
    end
  end

  private
  def self.get_status
    @html_doc.at_css("#ctl00_cphDefault_lblStatusSolicitacao").content
  end

  def self.get_proceeding
    if @html_doc.at_css("#ctl00_cphDefault_lblTramiteNaoEnviado")
      @html_doc.at_css("#ctl00_cphDefault_lblTramiteNaoEnviado").content
    elsif @node
      @node.css("td")[0].content
    end
  end

  def self.get_destination
    @node.css("td")[2].content unless @node.nil?
  end

  def self.get_last_update
    content = @node.css("td")[3].content unless @node.nil?
    unless content.nil?
      result = content.gsub(/^[\302\240|\s]*|[\302\240|\s]*$/, "")
      return nil if result.empty?
      Date.strptime(content, "%d/%m/%Y")
    end
  end

  def self.get_service
    @html_doc.at_css("#ctl00_cphDefault_lblServGrav").content
  end

  def self.get_due_date
    @node.css("td")[4].content unless @node.nil?
  end

  def self.remove_static_data(district)
    ["Porto Alegre", "Rio Grande do Sul", "RS", "Brasil", "Brazil"].each do |p|
      @changeable_address.gsub!(%r{(\s)?#{p}(([(,)?\s])|\s-)?}, "")
    end
    @changeable_address.gsub!(district, "")
  end

end