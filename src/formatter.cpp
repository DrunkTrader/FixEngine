#include "formatter.hpp"
#include <map>
#include <algorithm>

namespace fix {

    FormattedMessage Formatter::format(const FixMessage& message) const {
        FormattedMessage formatted;
        std::ostringstream header_oss, body_oss, trailer_oss;

        // Sort tags for consistent output
        std::vector<std::pair<int, std::string>> sorted_fields(message.begin(), message.end());
        std::sort(sorted_fields.begin(), sorted_fields.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        for (const auto& [tag, value] : sorted_fields) {
            std::string line = "  Tag " + std::to_string(tag) + " (" + getTagName(tag) + "): " + value + "\n";
            
            if (isHeaderTag(tag)) {
                header_oss << line;
            } else if (isTrailerTag(tag)) {
                trailer_oss << line;
            } else {
                body_oss << line;
            }
        }

        formatted.header = header_oss.str();
        formatted.body = body_oss.str();
        formatted.trailer = trailer_oss.str();
        
        std::ostringstream summary_oss;
        summary_oss << "FIX Message Summary:\n";
        summary_oss << "  Total fields: " << message.size() << "\n";
        
        auto begin_it = message.find(8);
        if (begin_it != message.end()) {
            summary_oss << "  BeginString: " << begin_it->second << "\n";
        }
        
        auto msg_type_it = message.find(35);
        if (msg_type_it != message.end()) {
            std::string msg_type_name;
            try {
                int msg_type_int = std::stoi(msg_type_it->second);
                msg_type_name = getTagName(msg_type_int);
            } catch (...) {
                msg_type_name = msg_type_it->second;
            }
            summary_oss << "  MsgType: " << msg_type_it->second << " (" << msg_type_name << ")\n";
        }

        formatted.summary = summary_oss.str();
        return formatted;
    }

    std::string Formatter::formatValidationResult(const ValidationResult& result, 
                                                   const std::string& raw_message) const {
        std::ostringstream oss;
        
        if (result.ok) {
            oss << "✓ Validation PASSED\n";
        } else {
            oss << "✗ Validation FAILED\n";
            oss << "  Errors (" << result.errors.size() << "):\n";
            for (std::size_t i = 0; i < result.errors.size(); ++i) {
                oss << "    [" << (i + 1) << "] Tag " << result.errors[i].tag 
                    << ": " << result.errors[i].message << "\n";
            }
        }

        if (!raw_message.empty()) {
            oss << "\nRaw message length: " << raw_message.size() << " bytes\n";
        }

        return oss.str();
    }

    std::string Formatter::formatDetailed(const FixMessage& message) const {
        std::ostringstream oss;
        oss << "=== Detailed FIX Message ===\n\n";

        std::vector<std::pair<int, std::string>> sorted_fields(message.begin(), message.end());
        std::sort(sorted_fields.begin(), sorted_fields.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        oss << std::left << std::setw(6) << "Tag" 
            << std::setw(25) << "Name" 
            << std::setw(12) << "Type"
            << "Value\n";
        oss << std::string(70, '-') << "\n";

        for (const auto& [tag, value] : sorted_fields) {
            oss << std::left << std::setw(6) << tag
                << std::setw(25) << getTagName(tag)
                << std::setw(12) << getTagType(tag)
                << value << "\n";
        }

        return oss.str();
    }

    std::string Formatter::toReadableString(const FixMessage& message) const {
        std::ostringstream oss;
        
        std::vector<std::pair<int, std::string>> sorted_fields(message.begin(), message.end());
        std::sort(sorted_fields.begin(), sorted_fields.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        for (const auto& [tag, value] : sorted_fields) {
            if (&sorted_fields.front() != &*sorted_fields.begin()) {
                oss << "|";
            }
            oss << tag << "=" << value;
        }

        return oss.str();
    }

    std::string Formatter::getTagName(int tag) const {
        static const std::map<int, std::string> tag_names = {
            {1, "Account"},
            {3, "RefSeqNum"},
            {4, "RefSenderCompID"},
            {5, "RefTargetCompID"},
            {6, "BrokerOfCredit"},
            {7, "ExecID"},
            {8, "BeginString"},
            {9, "BodyLength"},
            {10, "CheckSum"},
            {11, "ClOrdID"},
            {12, "Commission"},
            {14, "ExecTransType"},
            {15, "Currency"},
            {17, "ExecID"},
            {18, "ExecInst"},
            {19, "ExecRefID"},
            {20, "ExecType"},
            {21, "HandlInst"},
            {22, "SecurityIDSource"},
            {26, "OrderRate"},
            {31, "LastPx"},
            {32, "LastQty"},
            {33, "NoLinesOfText"},
            {34, "MsgSeqNum"},
            {35, "MsgType"},
            {37, "OrderID"},
            {38, "OrderQty"},
            {39, "OrdStatus"},
            {40, "OrdType"},
            {41, "OrigClOrdID"},
            {44, "Price"},
            {45, "RefSeqNum"},
            {48, "SecurityID"},
            {49, "SenderCompID"},
            {50, "SenderSubID"},
            {51, "SendingDate"},
            {52, "SendingTime"},
            {54, "Side"},
            {55, "Symbol"},
            {56, "TargetCompID"},
            {57, "TargetSubID"},
            {58, "Text"},
            {60, "TransactTime"},
            {62, "ValidUntilTime"},
            {64, "SettlDate"},
            {67, "NoOrders"},
            {70, "AllocID"},
            {72, "AllocTransType"},
            {73, "OrderListID"},
            {75, "TradeDate"},
            {78, "NoAllocs"},
            {79, "AllocAccount"},
            {80, "AllocQty"},
            {81, "ProcessCode"},
            {84, "CxlQty"},
            {85, "CxlTransType"},
            {87, "AllocStatus"},
            {88, "AllocRejCode"},
            {89, "Signature"},
            {90, "SecureDataLen"},
            {91, "SecureData"},
            {92, "BrokerOfCredit"},
            {93, "SignatureLength"},
            {94, "EmailType"},
            {95, "RawDataLength"},
            {96, "RawData"},
            {97, "PossResend"},
            {98, "EncryptMethod"},
            {99, "StopPx"},
            {100, "ExDestination"},
            {102, "CxlRejReason"},
            {103, "OrdRejReason"},
            {105, "IDSource"},
            {106, "Issuer"},
            {107, "SecurityDesc"},
            {108, "HeartBtInt"},
            {109, "ClientID"},
            {110, "MinQty"},
            {111, "MaxFloor"},
            {112, "TestReqID"},
            {113, "ReportToExch"},
            {114, "LocateReqd"},
            {115, "OnBehalfOfCompID"},
            {116, "OnBehalfOfSubID"},
            {117, "QuoteID"},
            {118, "NetMoney"},
            {119, "SettlCurrAmt"},
            {120, "SettlCurrency"},
            {121, "ForexReq"},
            {122, "OrigSendingTime"},
            {123, "GapFillFlag"},
            {124, "NoExecs"},
            {126, "ExpireTime"},
            {127, "DKReason"},
            {128, "DeliverToCompID"},
            {129, "DeliverToSubID"},
            {130, "IOINaturalFlag"},
            {131, "QuoteReqID"},
            {132, "BidPx"},
            {133, "OfferPx"},
            {134, "BidSize"},
            {135, "OfferSize"},
            {136, "NoMiscFees"},
            {137, "MiscFeeAmt"},
            {138, "MiscFeeCurr"},
            {139, "MiscFeeType"},
            {140, "PrevClosePx"},
            {141, "ResetSeqNumFlag"},
            {142, "SenderLocationID"},
            {143, "TargetLocationID"},
            {144, "OnBehalfOfLocationID"},
            {145, "DeliverToLocationID"},
            {146, "NoRelatedSym"},
            {147, "Subject"},
            {148, "Headline"},
            {149, "URLLink"},
            {150, "ExecType"},
            {151, "LeavesQty"},
            {152, "CumQty"},
            {153, "AvgPx"},
            {154, "DayOrderQty"},
            {155, "DayCumQty"},
            {156, "DayOrderQty2"},
            {157, "NetGrossInd"},
            {158, "OpenClose"},
            {159, "Text"},
            {160, "EncodedIssuerLen"},
            {161, "EncodedIssuer"},
            {162, "EncodedSecurityDescLen"},
            {163, "EncodedSecurityDesc"},
            {164, "EncodedSubjectLen"},
            {165, "EncodedSubject"},
            {166, "EncodedHeadlineLen"},
            {167, "EncodedHeadline"},
            {168, "EncodedTextLen"},
            {169, "EncodedText"},
            {170, "SettlInstID"},
            {171, "SettlInstTransType"},
            {172, "SettlInstRefID"},
            {173, "SettlInstMode"},
            {174, "SettlInstSource"},
            {175, "SettlInstMsgID"},
            {176, "SettlInstVersion"},
            {177, "SettlInstOtherSrc"},
            {178, "SettlInstGrpID"},
            {179, "SettlInstAssignID"},
            {180, "SettlInstEncPartyLen"},
            {181, "SettlInstEncParty"},
            {182, "SettlInstEncPartyRole"},
            {183, "SettlInstEncPartyQual"},
            {184, "SettlInstEncAltPartyLen"},
            {185, "SettlInstEncAltParty"},
            {186, "SettlInstEncAltPartyRole"},
            {187, "SettlInstEncAltPartyQual"},
            {188, "SettlInstEncInitPartyLen"},
            {189, "SettlInstEncInitParty"},
            {190, "SettlInstEncInitPartyRole"},
            {191, "SettlInstEncInitPartyQual"},
            {192, "SettlInstEncExecPartyLen"},
            {193, "SettlInstEncExecParty"},
            {194, "SettlInstEncExecPartyRole"},
            {195, "SettlInstEncExecPartyQual"},
            {196, "SettlInstEncClearPartyLen"},
            {197, "SettlInstEncClearParty"},
            {198, "SettlInstEncClearPartyRole"},
            {199, "SettlInstEncClearPartyQual"},
            {200, "MaturityMonthYear"},
            {201, "MaturityDay"},
            {202, "StrikePrice"},
            {203, "CoveredOrUncovered"},
            {204, "CustomerOrFirm"},
            {205, "MaturityMonthYear"},
            {206, "OptAttribute"},
            {207, "SecurityExchange"},
            {208, "NotifyBrokerOfCredit"},
            {209, "AllocHandlInst"},
            {210, "MaxShow"},
            {211, "PegDifference"},
            {212, "XmlDataLen"},
            {213, "XmlData"},
            {214, "SettlDetails"},
            {215, "TradingSessionID"},
            {216, "TradingSessionSubID"},
            {217, "TierCode"},
            {218, "PriceUnitOfMeasure"},
            {219, "QuantityUnitOfMeasure"},
            {220, "MaturityDate"},
            {221, "PutOrCall"},
            {222, "StrikeCurrency"},
            {223, "CouponRate"},
            {224, "SecurityExchange"},
            {225, "PositionLimit"},
            {226, "NTPositionLimit"},
            {227, "Issuer"},
            {228, "EncodedIssuerLen"},
            {229, "EncodedIssuer"},
            {230, "SecurityDesc"},
            {231, "EncodedSecurityDescLen"},
            {232, "EncodedSecurityDesc"},
            {233, "Pool"},
            {234, "ContractSettlMonth"},
            {235, "CPProgram"},
            {236, "CPReg"},
            {237, "SecurityType"},
            {238, "StrikeCurrency"},
            {239, "NoSecurityEvents"},
            {240, "SecurityEvent"},
            {241, "SecurityEventDate"},
            {242, "SecurityEventType"},
            {243, "SecurityEventVenue"},
            {244, "DatedDate"},
            {245, "InterestAccrualDate"},
            {246, "UnderlyingSymbol"},
            {247, "UnderlyingSecurityID"},
            {248, "UnderlyingProduct"},
            {249, "UnderlyingCFICode"},
            {250, "UnderlyingCountryOfIssue"},
            {251, "UnderlyingMaturityDate"},
            {252, "UnderlyingStrikePrice"},
            {253, "UnderlyingOptAttribute"},
            {254, "UnderlyingSecurityExchange"},
            {255, "UnderlyingSecurityDesc"},
            {256, "RatioQty"},
            {257, "SideValue1"},
            {258, "SideValue2"},
            {259, "RoundLot"},
            {260, "TradeCondition"},
            {261, "ContraTradeTime"},
            {262, "ContraTradeQty"},
            {263, "LiquidityNumSecurities"},
            {264, "MarketSegmentID"},
            {265, "MarketID"},
            {266, "PriceType"},
            {267, "YieldType"},
            {268, "Yield"},
            {269, "TradSesReqID"},
            {270, "TradingSessionID"},
            {271, "TradingSessionSubID"},
            {272, "Contrast"},
            {273, "Color"},
            {274, "TickDirection"},
            {275, "ChangeInPrice"},
            {276, "MatchType"},
            {277, "TradeID"},
            {278, "ParentOrderID"},
            {279, "TradeRequestID"},
            {280, "TradeRequestType"},
            {281, "PreviouslyReported"},
            {282, "PrimaryTrdType"},
            {283, "TradeOriginationDate"},
            {284, "ExecType"},
            {285, "TradeVolume"},
            {286, "TradeCondition"},
            {287, "LastMkt"},
            {288, "TradeDate"},
            {289, "SettlDate"},
            {290, "MatchStatus"},
            {291, "Rejected"},
            {292, "TradeRequestResult"},
            {293, "TradeRequestStatus"},
            {294, "TradeReportID"},
            {295, "TradeReportRefID"},
            {296, "SecondaryTradeReportID"},
            {297, "TradeReportType"},
            {298, "TradeReportTransType"},
            {299, "TradeHandlingInstr"},
            {300, "TransferReason"}
        };

        auto it = tag_names.find(tag);
        return (it != tag_names.end()) ? it->second : "Unknown";
    }

    std::string Formatter::getTagType(int tag) const {
        // Common FIX data types by tag ranges and specific tags
        if (tag == 8 || tag == 35 || tag == 49 || tag == 56 || tag == 11 || 
            tag == 55 || tag == 17 || tag == 37 || tag == 109 || tag == 115) {
            return "String";
        }
        if (tag == 9 || tag == 34 || tag == 38 || tag == 151 || tag == 152 || 
            tag == 108 || tag == 134 || tag == 135) {
            return "int";
        }
        if (tag == 44 || tag == 99 || tag == 132 || tag == 133 || tag == 153 || 
            tag == 202 || tag == 31 || tag == 32) {
            return "Price";
        }
        if (tag == 52 || tag == 60 || tag == 75 || tag == 200 || tag == 220) {
            return "Timestamp";
        }
        if (tag == 54 || tag == 40 || tag == 39 || tag == 150 || tag == 201 || 
            tag == 203 || tag == 121 || tag == 97) {
            return "char";
        }
        if (tag == 10) {
            return "Checksum";
        }
        return "String";
    }

    bool Formatter::isHeaderTag(int tag) const {
        // Standard FIX header tags
        static const std::set<int> header_tags = {
            8,   // BeginString
            9,   // BodyLength
            35,  // MsgType
            49,  // SenderCompID
            56,  // TargetCompID
            115, // OnBehalfOfCompID
            128, // DeliverToCompID
            34,  // MsgSeqNum
            52,  // SendingTime
            97,  // PossResend
            98   // EncryptMethod
        };
        return header_tags.count(tag) > 0;
    }

    bool Formatter::isTrailerTag(int tag) const {
        // Standard FIX trailer tags
        static const std::set<int> trailer_tags = {
            10,  // CheckSum
            89,  // Signature
            90,  // SecureDataLen
            91,  // SecureData
            93,  // SignatureLength
            95,  // RawDataLength
            96   // RawData
        };
        return trailer_tags.count(tag) > 0;
    }

}
