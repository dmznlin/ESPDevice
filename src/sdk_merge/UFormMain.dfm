object fFormMain: TfFormMain
  Left = 0
  Top = 0
  Caption = 'SDK-Merger'
  ClientHeight = 606
  ClientWidth = 813
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 12
  object PanelTop: TdxPanel
    Left = 0
    Top = 0
    Width = 813
    Height = 337
    Align = alTop
    TabOrder = 0
    object MemoSrc2: TUSynMemo
      Left = 409
      Top = 0
      Width = 402
      Height = 304
      Align = alClient
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Courier New'
      Font.Style = []
      TabOrder = 0
      Gutter.Font.Charset = DEFAULT_CHARSET
      Gutter.Font.Color = clWindowText
      Gutter.Font.Height = -11
      Gutter.Font.Name = 'Courier New'
      Gutter.Font.Style = []
      Gutter.ShowLineNumbers = True
      Lines.Strings = (
        'MemoSrc2')
      Options = [eoAutoIndent, eoDragDropEditing, eoDropFiles, eoEnhanceEndKey, eoGroupUndo, eoScrollPastEol, eoShowScrollHint, eoSmartTabDelete, eoSmartTabs, eoTabsToSpaces]
      OnDropFiles = MemoSrc1DropFiles
    end
    object MemoSrc1: TUSynMemo
      Left = 0
      Top = 0
      Width = 401
      Height = 304
      Align = alLeft
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Courier New'
      Font.Style = []
      TabOrder = 1
      Gutter.Font.Charset = DEFAULT_CHARSET
      Gutter.Font.Color = clWindowText
      Gutter.Font.Height = -11
      Gutter.Font.Name = 'Courier New'
      Gutter.Font.Style = []
      Gutter.ShowLineNumbers = True
      Lines.Strings = (
        'USynMemo1')
      Options = [eoAutoIndent, eoDragDropEditing, eoDropFiles, eoEnhanceEndKey, eoGroupUndo, eoScrollPastEol, eoShowScrollHint, eoSmartTabDelete, eoSmartTabs, eoTabsToSpaces]
      OnDropFiles = MemoSrc1DropFiles
    end
    object cxSplitter1: TcxSplitter
      Left = 401
      Top = 0
      Width = 8
      Height = 304
      HotZoneClassName = 'TcxXPTaskBarStyle'
      OnMoved = cxSplitter1Moved
      OnBeforeClose = cxSplitter1BeforeClose
    end
    object dxPanel2: TdxPanel
      Left = 0
      Top = 304
      Width = 811
      Height = 31
      Align = alBottom
      Frame.Visible = False
      TabOrder = 3
      DesignSize = (
        811
        31)
      object BtnMerge: TcxButton
        Left = 371
        Top = 3
        Width = 75
        Height = 25
        Caption = #21512#24182
        TabOrder = 0
        OnClick = BtnMergeClick
      end
      object Bar1: TcxProgressBar
        Left = 603
        Top = 6
        Anchors = [akTop, akRight]
        TabOrder = 1
        Visible = False
        Width = 200
      end
      object RadioAB: TcxRadioButton
        Left = 5
        Top = 7
        Caption = '#define key value'
        Checked = True
        TabOrder = 2
        TabStop = True
        AutoSize = True
        Transparent = True
      end
      object RadioKV: TcxRadioButton
        Left = 132
        Top = 7
        Caption = 'key = value'
        TabOrder = 3
        AutoSize = True
        Transparent = True
      end
    end
  end
  object cxSplitter2: TcxSplitter
    Left = 0
    Top = 337
    Width = 813
    Height = 8
    HotZoneClassName = 'TcxXPTaskBarStyle'
    AlignSplitter = salTop
  end
  object MemoDest: TUSynMemo
    Left = 0
    Top = 345
    Width = 813
    Height = 261
    Align = alClient
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Courier New'
    Font.Style = []
    TabOrder = 2
    Gutter.Font.Charset = DEFAULT_CHARSET
    Gutter.Font.Color = clWindowText
    Gutter.Font.Height = -11
    Gutter.Font.Name = 'Courier New'
    Gutter.Font.Style = []
    Gutter.ShowLineNumbers = True
    Lines.Strings = (
      'USynMemo1')
  end
end
