class Variable
  attr_reader :Name, :Type, :Default, :NonMethodParam, :AngelScriptRef, :AngelScriptUseInstead,
              :MemberAccess, :NonSerializeParam, :NoConst

  def initialize(name, type, default: nil, noRef: false, noConst: false, nonMethodParam: false,
                 move: false, serializeas: nil,
                 angelScriptRef: 'in', angelScriptUseInstead: nil, memberaccess: nil,
                 nonserializeparam: false)

    @Name = name
    @Type = type
    @Default = default
    # This is for accessing this parameter from the finished object, instead of the normal
    # way around
    @MemberAccess = memberaccess
    @NonSerializeParam = nonserializeparam

    unless @Default.nil?
      if @Default == true
        @Default = 'true'
      elsif @Default == false
        @Default = 'false'
      end

      # Empty string is output as two quotes
      @Default = %("") if @Default.is_a?(String) && (@Default == '')
    end

    @NoRef = noRef
    @NoConst = noConst
    @NonMethodParam = nonMethodParam
    @Move = move
    @SerializeAs = serializeas
    @AngelScriptRef = angelScriptRef
    @AngelScriptUseInstead = angelScriptUseInstead
  end

  def formatDefinition
    "#{@Type} #{@Name}#{formatDefault(header: true)};"
  end

  def formatDefault(opts)
    if @Default.nil?
      ''
    else
      if opts.include?(:header)
        ' = ' + @Default
      else
        "/* = #{@Default} */"
      end
    end
  end

  # Formats full definition for use in parameter list
  def formatForParams(opts)
    if @NoRef
      "#{@Type} #{@Name.downcase}#{formatDefault opts}"
    elsif @Move
      "#{@Type}&& #{@Name.downcase}#{formatDefault opts}"
    else
      if @NoConst
        "#{@Type} &#{@Name.downcase}#{formatDefault opts}"
      else
        "const #{@Type} &#{@Name.downcase}#{formatDefault opts}"
      end
    end
  end

  # Formats only the type. Useful for use in templates
  def formatType
    if @NoRef
      @Type.to_s
    elsif @Move
      "#{@Type}&&"
    else
      if @NoConst
        "#{@Type}&"
      else
        "const #{@Type}&"
      end
    end
  end

  # Formatting for angelscript bindings
  def TypeAS
    # Standard translations that reduce typing
    case @Type
    when 'std::string'
      'string'
    when 'uint8_t'
      'uint8'
    when 'uint16_t'
      'uint16'
    when 'uint32_t'
      'uint32'
    when 'int8_t'
      'int8'
    when 'int16_t'
      'int16'
    when 'int32_t'
      'int32'
    else
      @Type.sub('*', '@')
    end
  end

  # Formats full definition for use in parameter list
  def formatForParamsAngelScript
    return @AngelScriptUseInstead.formatForParamsAngelScript if @AngelScriptUseInstead

    opts = { header: true }
    if @NoRef
      "#{self.TypeAS} #{@Name.downcase}#{formatDefault opts}"
    elsif @Move
      "#{self.TypeAS}&& #{@Name.downcase}#{formatDefault opts}"
    else
      if @NoConst
        "#{self.TypeAS} &#{@AngelScriptRef} #{@Name.downcase}#{formatDefault opts}"
      else
        "const #{self.TypeAS} &#{@AngelScriptRef} #{@Name.downcase}#{formatDefault opts}"
      end
    end
  end

  def formatForArgumentList
    if !@NonMethodParam
      @Name.downcase.to_s
    else
      @Name.to_s
    end
  end

  def formatInitializer
    if @Move
      # Move constructor
      "#{@Name}(std::move(#{@Name.downcase}))"
    else
      "#{@Name}(#{@Name.downcase})"
    end
  end

  def formatSerializer
    if @SerializeAs.nil?
      @Name.to_s
    else
      "static_cast<#{@SerializeAs}>(#{@Name})"
    end
  end

  def formatMemberSerializer(variable)
    if @SerializeAs.nil?
      (variable + @MemberAccess).to_s
    else
      "static_cast<#{@SerializeAs}>(#{variable + @MemberAccess})"
    end
  end

  def formatCopy
    "#{@Name}(other.#{@Name})"
  end

  def formatMove
    "#{@Name}(std::move(other.#{@Name}))"
  end

  def formatDeserializer(packetname, target: nil)
    target ||= @Name

    if @SerializeAs.nil?
      "#{packetname} >> #{target};"
    else
      tempName = 'temp_' + target
      str = "#{@SerializeAs} #{tempName};\n"
      str += "#{packetname} >> #{tempName};\n"
      str += "#{target} = static_cast<#{@Type}>(#{tempName});\n"
      str
    end
  end

  def needs_wrapper
    @Type.include? '::pointer'
  end

  def plain_type
    @Type.gsub '::pointer', ''
  end

  def format_type_wrapped
    if needs_wrapper
      "#{plain_type}*"
    else
      formatType
    end
  end

  def format_for_wrapped_parameters(opts)
    if needs_wrapper
      "#{plain_type}* #{@Name.downcase}"
    else
      formatForParams opts
    end
  end

  def format_for_wrapped_argument_list
    if needs_wrapper
      if !@NonMethodParam
        @Name.downcase.to_s
      else
        "#{plain_type}::WrapPtr<#{plain_type}>(#{@Name.downcase}"
      end
    else
      formatForArgumentList
    end
  end

  def format_for_params_angelscript_wrapped
    if needs_wrapper
      "#{plain_type}@ #{@Name.downcase}"
    else
      formatForParamsAngelScript
    end
  end
end
