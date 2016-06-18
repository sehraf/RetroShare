
/*
 * libretroshare/src/serialiser: rstlvtypes.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Chris Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "rstlvkeys.h"
#include "rstlvbase.h"
#include "rsbaseserial.h"
#include "util/stacktrace.h"
#include "gxs/gxssecurity.h"

#include <iostream>

#define TLV_DEBUG 1

// This should be removed eventually, but will break backward compatibility
#define KEEP_OLD_SIGNATURE_SERIALISE_FORMAT

/************************************* RsTlvSecurityKey ************************************/

RsTlvRSAKey::RsTlvRSAKey()
	:RsTlvItem(), keyFlags(0), startTS(0), endTS(0),  keyData(TLV_TYPE_KEY_EVP_PKEY)
{
	return;
}

void RsTlvRSAKey::TlvClear()
{
	keyId.clear();
	keyFlags = 0;
	startTS = 0;
	endTS = 0;
	keyData.TlvClear();
}

/* clears keyData - but doesn't delete */
void RsTlvRSAKey::ShallowClear()
{
	keyId.clear();
	keyFlags = 0;
	startTS = 0;
	endTS = 0;
	keyData.bin_data = 0;
	keyData.bin_len = 0;
}

uint32_t RsTlvRSAKey::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header + 4 for size */

	/* now add comment and title length of this tlv object */

#ifdef KEEP_OLD_SIGNATURE_SERIALISE_FORMAT
	s += GetTlvStringSize(keyId.toStdString()) ;
#else
	s += keyId.serial_size(); 
#endif
	s += 4;
	s += 4;
	s += 4;
	s += keyData.TlvSize();

	return s;
}

bool  RsTlvRSAKey::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
		std::cerr << "RsTlvSecurityKey::SetTlv() Failed not enough space";
		std::cerr << std::endl;
		return false; /* not enough space */
	}

	bool ok = checkKey();	// check before serialise, just in case

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_SECURITY_KEY, tlvsize);
#ifdef KEEP_OLD_SIGNATURE_SERIALISE_FORMAT
	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, keyId.toStdString());  
#else
	ok &= keyId.serialise(data, tlvend, *offset) ;
#endif
	ok &= setRawUInt32(data, tlvend, offset, keyFlags);
	ok &= setRawUInt32(data, tlvend, offset, startTS);
	ok &= setRawUInt32(data, tlvend, offset, endTS);
	ok &= keyData.SetTlv(data, tlvend, offset);  

	return ok;

}

bool  RsTlvRSAKey::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Fail, not enough space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	if (tlvtype != TLV_TYPE_SECURITY_KEY) /* check type */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Fail, wrong type";
		std::cerr << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;
#ifdef KEEP_OLD_SIGNATURE_SERIALISE_FORMAT
	std::string s ;
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, s);  
	keyId = RsGxsId(s) ;
#else
	ok &= keyId.deserialise(data, tlvend, *offset) ;
#endif
	ok &= getRawUInt32(data, tlvend, offset, &(keyFlags));
	ok &= getRawUInt32(data, tlvend, offset, &(startTS));
	ok &= getRawUInt32(data, tlvend, offset, &(endTS));
	ok &= keyData.GetTlv(data, tlvend, offset);  

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	if (!ok)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Failed somewhere ok == false";
		std::cerr << std::endl;
#endif
	}
	return ok && checkKey() ;
}

std::ostream& RsTlvRSAKey::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvSecurityKey", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "KeyId:" << keyId;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "KeyFlags:" << keyFlags;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "StartTS:" << startTS;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "EndTS:" << endTS;
	out << std::endl;

	keyData.print(out, int_Indent);

	out << std::endl;
	
	printEnd(out, "RsTlvSecurityKey", indent);
	return out;
}


bool RsTlvPrivateRSAKey::checkKey() const
{ 
    return bool(keyFlags & RSTLV_KEY_TYPE_FULL) && !bool(keyFlags & RSTLV_KEY_TYPE_PUBLIC_ONLY) && GxsSecurity::checkPrivateKey(*this) ;
}

bool RsTlvPublicRSAKey::checkKey() const
{ 
    return bool(keyFlags & RSTLV_KEY_TYPE_PUBLIC_ONLY) && !bool(keyFlags & RSTLV_KEY_TYPE_FULL) && GxsSecurity::checkPublicKey(*this) ;
}

/************************************* RsTlvSecurityKeySet ************************************/

void RsTlvSecurityKeySet::TlvClear()
{
	groupId.clear();
#ifdef TODO
	public_keys.clear(); //empty list
	private_keys.clear(); //empty list
#endif
}

uint32_t RsTlvSecurityKeySet::TlvSize() const
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	s += GetTlvStringSize(groupId); 

	for(std::map<RsGxsId, RsTlvPublicRSAKey>::const_iterator it = public_keys.begin(); it != public_keys.end() ; ++it)
		s += (it->second).TlvSize();
    
	for(std::map<RsGxsId, RsTlvPrivateRSAKey>::const_iterator it = private_keys.begin(); it != private_keys.end() ; ++it)
		s += (it->second).TlvSize();

	return s;
}

bool  RsTlvSecurityKeySet::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKeySet::SetTlv() Failed not enough space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_SECURITYKEYSET , tlvsize);
	
	/* groupId */
	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_GROUPID, groupId);

    	for(std::map<RsGxsId, RsTlvPublicRSAKey>::const_iterator it = public_keys.begin(); it != public_keys.end() ; ++it)
			ok &= (it->second).SetTlv(data, size, offset);
        
    	for(std::map<RsGxsId, RsTlvPrivateRSAKey>::const_iterator it = private_keys.begin(); it != private_keys.end() ; ++it)
			ok &= (it->second).SetTlv(data, size, offset);

	return ok;
}


bool  RsTlvSecurityKeySet::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_SECURITYKEYSET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	/* groupId */
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_GROUPID, groupId);

	/* while there is TLV  */
	while((*offset) + 2 < tlvend)
	{
		/* get the next type */
		uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );

                // Security key set can be composed of public or private keys. We sort them into the correct bins after deserialisation
                
		switch(tlvsubtype)
		{
		case TLV_TYPE_SECURITY_KEY:
		{
			uint32_t offset_save = *offset ;

			RsTlvPublicRSAKey public_key;

			if(public_key.GetTlv(data, tlvend, offset))
				public_keys[public_key.keyId] = public_key;
			else
			{
				*offset = offset_save ;

				RsTlvPrivateRSAKey private_key;

				if(private_key.GetTlv(data, tlvend, offset))
					private_keys[private_key.keyId] = private_key;
			}
		}
			break ;

		default:
			ok &= SkipUnknownTlv(data, tlvend, offset);
			break;

		}

		if (!ok)
			break;
	}



	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKeySet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}


// prints out contents of RsTlvSecurityKeySet
std::ostream &RsTlvSecurityKeySet::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvSecurityKeySet", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "GroupId: " << groupId;
	out << std::endl;

	for( std::map<RsGxsId, RsTlvPublicRSAKey>::const_iterator it = public_keys.begin(); it != public_keys.end() ; ++it)
		(it->second).print(out, int_Indent);
	for( std::map<RsGxsId, RsTlvPrivateRSAKey>::const_iterator it = private_keys.begin(); it != private_keys.end() ; ++it)
		(it->second).print(out, int_Indent);

	printEnd(out, "RsTlvSecurityKeySet", indent);
	return out;
}


/************************************* RsTlvSecurityKey ************************************/

RsTlvKeySignature::RsTlvKeySignature()
	:RsTlvItem(), signData(TLV_TYPE_SIGN_RSA_SHA1)
{
	ShallowClear() ;	// avoids uninitialized memory if the fields are not initialized.
}

void RsTlvKeySignature::TlvClear()
{
	keyId.clear();
	signData.TlvClear();
}

/* clears signData - but doesn't delete */
void RsTlvKeySignature::ShallowClear()
{
	keyId.clear();
	signData.bin_data = 0;
	signData.bin_len = 0;
}

uint32_t RsTlvKeySignature::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header + 4 for size */

#ifdef KEEP_OLD_SIGNATURE_SERIALISE_FORMAT
	s += GetTlvStringSize(keyId.toStdString()) ;
#else
	s += keyId.serial_size() ;
#endif
	s += signData.TlvSize();
	return s;

}

bool  RsTlvKeySignature::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::SetTlv() Fail, not enough space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_KEYSIGNATURE, tlvsize);

#ifdef KEEP_OLD_SIGNATURE_SERIALISE_FORMAT
	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, keyId.toStdString());  
#else
	ok &= keyId.serialise(data, tlvend, *offset) ;
#endif
	ok &= signData.SetTlv(data, tlvend, offset);  

	if (!ok)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::SetTlv() Failed somewhere";
		std::cerr << std::endl;
#endif
	}

	return ok;

}


bool  RsTlvKeySignature::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::GetTlv() Not Enough Space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	if (tlvtype != TLV_TYPE_KEYSIGNATURE) /* check type */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::GetTlv() Type Fail";
		std::cerr << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

#ifdef KEEP_OLD_SIGNATURE_SERIALISE_FORMAT
	std::string s ;
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, s);  
	keyId = RsGxsId(s) ;
#else
	ok &= keyId.deserialise(data, tlvend, *offset) ;
#endif
	ok &= signData.GetTlv(data, tlvend, offset);  

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvKeySignature::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvKeySignature", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "KeyId:" << keyId;
	out << std::endl;

	signData.print(out, int_Indent);

	out << std::endl;
	
	printEnd(out, "RsTlvKeySignature", indent);
	return out;
}


/************************************* RsTlvKeySignatureSet ************************************/

RsTlvKeySignatureSet::RsTlvKeySignatureSet()
{

}

std::ostream &RsTlvKeySignatureSet::print(std::ostream &out, uint16_t indent) const
{
    printBase(out, "RsTlvKeySignatureSet", indent);
    uint16_t int_Indent = indent + 2;

    printIndent(out, int_Indent);

    std::map<SignType, RsTlvKeySignature>::const_iterator mit = keySignSet.begin();

    for(; mit != keySignSet.end(); ++mit)
    {
        out << "SignType: " << mit->first << std::endl;
        const RsTlvKeySignature& sign = mit->second;
        sign.print(out, indent);
    }

    out << std::endl;

    printEnd(out, "RsTlvKeySignatureSet", indent);
    return out;
}

void RsTlvKeySignatureSet::TlvClear()
{
    keySignSet.clear();
}

bool RsTlvKeySignatureSet::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{

    /* must check sizes */
    uint32_t tlvsize = TlvSize();
    uint32_t tlvend  = *offset + tlvsize;

    if (size < tlvend)
    {
#ifdef TLV_DEBUG
            std::cerr << "RsTlvKeySignatureSet::SetTlv() Failed not enough space";
            std::cerr << std::endl;
#endif
            return false; /* not enough space */
    }

    bool ok = true;

            /* start at data[offset] */
    ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_KEYSIGNATURESET , tlvsize);


    if(!keySignSet.empty())
    {
            std::map<SignType, RsTlvKeySignature>::const_iterator it;

            for(it = keySignSet.begin(); it != keySignSet.end() ; ++it)
            {
                ok &= SetTlvUInt32(data, size, offset, TLV_TYPE_KEYSIGNATURETYPE, it->first);
                ok &= (it->second).SetTlv(data, size, offset);
            }
    }


return ok;
}

bool RsTlvKeySignatureSet::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
    if (size < *offset + TLV_HEADER_SIZE)
            return false;

    uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
    uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
    uint32_t tlvend = *offset + tlvsize;

    if (size < tlvend)    /* check size */
            return false; /* not enough space */

    if (tlvtype != TLV_TYPE_KEYSIGNATURESET) /* check type */
            return false;

    bool ok = true;

    /* ready to load */
    TlvClear();

    /* skip the header */
    (*offset) += TLV_HEADER_SIZE;

    SignType sign_type = 0;

    /* while there is TLV  */
    while((*offset) + 2 < tlvend)
    {

            /* get the next type */
            uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );

            switch(tlvsubtype)
            {
                    case TLV_TYPE_KEYSIGNATURE:
                    {
                            RsTlvKeySignature sign;
                            ok &= sign.GetTlv(data, size, offset);
                            if (ok)
                            {
                                keySignSet[sign_type] = sign;
                            }
                    }
                    break;
                    case TLV_TYPE_KEYSIGNATURETYPE:
                    {
                        ok = GetTlvUInt32(data, size, offset, TLV_TYPE_KEYSIGNATURETYPE, &sign_type);
                    }
                    break;
                    default:
                        ok &= SkipUnknownTlv(data, tlvend, offset);
                        break;
            }

            if (!ok)
                    break;
    }



    /***************************************************************************
     * NB: extra components could be added (for future expansion of the type).
     *            or be present (if this code is reading an extended version).
     *
     * We must chew up the extra characters to conform with TLV specifications
     ***************************************************************************/
    if (*offset != tlvend)
    {
#ifdef TLV_DEBUG
            std::cerr << "RsTlvKeySignatureSet::GetTlv() Warning extra bytes at end of item";
            std::cerr << std::endl;
#endif
            *offset = tlvend;
    }

    return ok;
}

uint32_t RsTlvKeySignatureSet::TlvSize() const
{
    uint32_t s = TLV_HEADER_SIZE; // header size
    std::map<SignType, RsTlvKeySignature>::const_iterator it;

    for(it = keySignSet.begin(); it != keySignSet.end() ; ++it)
    {
        s += GetTlvUInt32Size(); // sign type
        s += it->second.TlvSize(); // signature
    }

    return s;
}




















