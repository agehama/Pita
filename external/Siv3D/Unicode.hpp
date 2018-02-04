//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2018 Ryo Suzuki
//	Copyright (c) 2016-2018 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

#pragma once
#include <string>
#include <array>

namespace cgl
{
	namespace Unicode
	{
		[[nodiscard]] inline constexpr bool IsHighSurrogate(const char16_t ch) noexcept
		{
			return (0xD800 <= ch) && (ch <= 0xDBFF);
		}

		[[nodiscard]] inline constexpr bool IsLowSurrogate(const char16_t ch) noexcept
		{
			return (0xDC00 <= ch) && (ch <= 0xDFFF);
		}

		/// <summary>
		/// UTF-8 文字列を UTF-16 文字列に変換します。
		/// </summary>
		/// <param name="str">
		/// UTF-8 文字列
		/// </param>
		/// <returns>
		/// 変換された文字列
		/// </returns>
		[[nodiscard]] std::u16string UTF8ToUTF16(const std::string& str);

		/// <summary>
		/// UTF-8 文字列を UTF-32 文字列に変換します。
		/// </summary>
		/// <param name="str">
		/// UTF-8 文字列
		/// </param>
		/// <returns>
		/// 変換された文字列
		/// </returns>
		[[nodiscard]] std::u32string UTF8ToUTF32(const std::string& str);

		/// <summary>
		/// UTF-16 文字列を UTF-8 文字列に変換します。
		/// </summary>
		/// <param name="str">
		/// UTF-16 文字列
		/// </param>
		/// <returns>
		/// 変換された文字列
		/// </returns>
		[[nodiscard]] std::string UTF16ToUTF8(const std::u16string& str);

		/// <summary>
		/// UTF-16 文字列を UTF-32 文字列に変換します。
		/// </summary>
		/// <param name="str">
		/// UTF-16 文字列
		/// </param>
		/// <returns>
		/// 変換された文字列
		/// </returns>
		[[nodiscard]] std::u32string UTF16ToUTF32(const std::u16string& str);

		/// <summary>
		/// UTF-32 文字列を UTF-8 文字列に変換します。
		/// </summary>
		/// <param name="str">
		/// UTF-16 文字列
		/// </param>
		/// <returns>
		/// 変換された文字列
		/// </returns>
		[[nodiscard]] std::string UTF32ToUTF8(const std::u32string& str);

		/// <summary>
		/// UTF-32 文字列を UTF-16 文字列に変換します。
		/// </summary>
		/// <param name="str">
		/// UTF-16 文字列
		/// </param>
		/// <returns>
		/// 変換された文字列
		/// </returns>
		[[nodiscard]] std::u16string UTF32ToUTF16(const std::u32string& str);

		[[nodiscard]] size_t CountCodePoints(const std::string& str) noexcept;

		[[nodiscard]] size_t CountCodePoints(const std::u16string& str) noexcept;

		struct Translator_UTF8toUTF32
		{
		private:

			std::array<char, 4> m_buffer;

			uint32_t m_count = 0;

			char32_t m_result = 0;

		public:

			[[nodiscard]] bool put(char code) noexcept;

			[[nodiscard]] char32_t get() const noexcept
			{
				return m_result;
			}
		};

		struct Translator_UTF16toUTF32
		{
		private:

			char32_t m_result = 0;

			char16_t m_buffer = 0;

			bool m_hasHighSurrogate = false;

		public:

			[[nodiscard]] bool put(char16_t code) noexcept;

			[[nodiscard]] char32_t get() const noexcept
			{
				return m_result;
			}
		};

		struct Translator_UTF32toUTF8
		{
		private:

			std::array<char, 4> m_buffer;

		public:

			[[nodiscard]] size_t put(char32_t code) noexcept;

			[[nodiscard]] const std::array<char, 4>& get() const noexcept
			{
				return m_buffer;
			}

			[[nodiscard]] std::array<char, 4>::const_iterator begin() const noexcept
			{
				return m_buffer.begin();
			}
		};

		struct Translator_UTF32toUTF16
		{
		private:

			std::array<char16_t, 2> m_buffer;

		public:

			[[nodiscard]] size_t put(char32_t code) noexcept;

			[[nodiscard]] const std::array<char16_t, 2>& get() const noexcept
			{
				return m_buffer;
			}

			[[nodiscard]] std::array<char16_t, 2>::const_iterator begin() const noexcept
			{
				return m_buffer.begin();
			}
		};
	}
}

//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2018 Ryo Suzuki
//	Copyright (c) 2016-2018 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

#include "miniutf.hpp"

namespace cgl
{
	namespace detail
	{
		[[nodiscard]] inline size_t UTF8Length(const char32_t codePoint) noexcept
		{
			if (codePoint < 0x80)
			{
				return 1;
			}
			else if (codePoint < 0x800)
			{
				return 2;
			}
			else if (codePoint < 0x10000)
			{
				return 3;
			}
			else if (codePoint < 0x110000)
			{
				return 4;
			}
			else
			{
				return 3; // U+FFFD
			}
		}

		[[nodiscard]] inline size_t UTF8Length(const std::u32string& str) noexcept
		{
			size_t length = 0;

			const char32_t* pSrc = str.c_str();
			const char32_t* const pSrcEnd = pSrc + str.size();

			while (pSrc != pSrcEnd)
			{
				length += UTF8Length(*pSrc++);
			}

			return length;
		}

		[[nodiscard]] static size_t UTF8Length(const std::u16string& str) noexcept
		{
			size_t length = 0;

			const char16_t* pSrc = str.c_str();
			const char16_t* const pSrcEnd = pSrc + str.size();

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				length += UTF8Length(utf16_decode(pSrc, pSrcEnd - pSrc, offset));

				pSrc += offset;
			}

			return length;
		}

		static void UTF8Encode(char** s, const char32_t codePoint) noexcept
		{
			if (codePoint < 0x80)
			{
				*(*s)++ = static_cast<char>(codePoint);
			}
			else if (codePoint < 0x800)
			{
				*(*s)++ = static_cast<char>((codePoint >> 6) | 0xC0);
				*(*s)++ = static_cast<char>((codePoint & 0x3F) | 0x80);
			}
			else if (codePoint < 0x10000)
			{
				*(*s)++ = static_cast<char>((codePoint >> 12) | 0xE0);
				*(*s)++ = static_cast<char>(((codePoint >> 6) & 0x3F) | 0x80);
				*(*s)++ = static_cast<char>((codePoint & 0x3F) | 0x80);
			}
			else if (codePoint < 0x110000)
			{
				*(*s)++ = static_cast<char>((codePoint >> 18) | 0xF0);
				*(*s)++ = static_cast<char>(((codePoint >> 12) & 0x3F) | 0x80);
				*(*s)++ = static_cast<char>(((codePoint >> 6) & 0x3F) | 0x80);
				*(*s)++ = static_cast<char>((codePoint & 0x3F) | 0x80);
			}
			else
			{
				*(*s)++ = static_cast<std::uint8_t>(0xEF);
				*(*s)++ = static_cast<std::uint8_t>(0xBF);
				*(*s)++ = static_cast<std::uint8_t>(0xBD);
			}
		}

		[[nodiscard]] inline size_t UTF16Length(const char32_t codePoint) noexcept
		{
			if (codePoint < 0x10000)
			{
				return 1;
			}
			else if (codePoint < 0x110000)
			{
				return 2;
			}
			else
			{
				return 1; // 0xFFFD
			}
		}

		[[nodiscard]] inline size_t UTF16Length(const std::u32string& str) noexcept
		{
			size_t length = 0;

			const char32_t* pSrc = str.c_str();
			const char32_t* const pSrcEnd = pSrc + str.size();

			while (pSrc != pSrcEnd)
			{
				length += UTF16Length(*pSrc++);
			}

			return length;
		}

		[[nodiscard]] inline size_t UTF16Length(const std::string& str) noexcept
		{
			size_t length = 0;

			const char* pSrc = str.c_str();
			const char* const pSrcEnd = pSrc + str.size();

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset = 0;

				length += UTF16Length(detail::utf8_decode(pSrc, pSrcEnd - pSrc, offset));

				pSrc += offset;
			}

			return length;
		}

		static void UTF16Encode(char16_t** s, const char32_t codePoint) noexcept
		{
			if (codePoint < 0x10000)
			{
				*(*s)++ = static_cast<char16_t>(codePoint);
			}
			else if (codePoint < 0x110000)
			{
				*(*s)++ = static_cast<char16_t>(((codePoint - 0x10000) >> 10) + 0xD800);
				*(*s)++ = static_cast<char16_t>((codePoint & 0x3FF) + 0xDC00);
			}
			else
			{
				*(*s)++ = static_cast<char16_t>(0xFFFD);
			}
		}

		[[nodiscard]] static size_t UTF32Length(const std::string& str) noexcept
		{
			size_t length = 0;

			const char* pSrc = str.c_str();
			const char* const pSrcEnd = pSrc + str.size();

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				utf8_decode(pSrc, pSrcEnd - pSrc, offset);

				pSrc += offset;

				++length;
			}

			return length;
		}

		[[nodiscard]] static size_t UTF32Length(const std::u16string& str)
		{
			size_t length = 0;

			const char16_t* pSrc = str.c_str();
			const char16_t* const pSrcEnd = pSrc + str.size();

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				utf16_decode(pSrc, pSrcEnd - pSrc, offset);

				pSrc += offset;

				++length;
			}

			return length;
		}
	}
}

namespace cgl
{
	namespace Unicode
	{
		inline std::u16string UTF8ToUTF16(const std::string& str)
		{
			std::u16string result(detail::UTF16Length(str), '0');

			const char* pSrc = str.c_str();
			const char* const pSrcEnd = pSrc + str.size();
			char16_t* pDst = &result[0];

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				detail::UTF16Encode(&pDst, detail::utf8_decode(pSrc, pSrcEnd - pSrc, offset));

				pSrc += offset;
			}

			return result;
		}

		inline std::u32string UTF8ToUTF32(const std::string& str)
		{
			std::u32string result(detail::UTF32Length(str), '0');
			
			const char* pSrc = str.c_str();
			const char* const pSrcEnd = pSrc + str.size();
			char32_t* pDst = &result[0];

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				*pDst++ = detail::utf8_decode(pSrc, pSrcEnd - pSrc, offset);

				pSrc += offset;
			}

			return result;
		}

		inline std::string UTF16ToUTF8(const std::u16string& str)
		{
			std::string result(detail::UTF8Length(str), '0');
			
			const char16_t* pSrc = str.c_str();
			const char16_t* const pSrcEnd = pSrc + str.size();
			char* pDst = &result[0];

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				detail::UTF8Encode(&pDst, detail::utf16_decode(pSrc, pSrcEnd - pSrc, offset));

				pSrc += offset;
			}

			return result;
		}

		inline std::u32string UTF16ToUTF32(const std::u16string& str)
		{
			std::u32string result(detail::UTF32Length(str), '0');
			
			const char16_t* pSrc = str.c_str();
			const char16_t* const pSrcEnd = pSrc + str.size();
			char32_t* pDst = &result[0];

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				*pDst++ = detail::utf16_decode(pSrc, pSrcEnd - pSrc, offset);

				pSrc += offset;
			}

			return result;
		}

		inline std::string UTF32ToUTF8(const std::u32string& str)
		{
			std::string result(detail::UTF8Length(str), '0');

			const char32_t* pSrc = str.c_str();
			const char32_t* const pSrcEnd = pSrc + str.size();
			char* pDst = &result[0];

			while (pSrc != pSrcEnd)
			{
				detail::UTF8Encode(&pDst, *pSrc++);
			}

			return result;
		}

		inline std::u16string UTF32ToUTF16(const std::u32string& str)
		{
			std::u16string result(detail::UTF16Length(str), u'0');

			const char32_t* pSrc = str.c_str();
			const char32_t* const pSrcEnd = pSrc + str.size();
			char16_t* pDst = &result[0];

			while (pSrc != pSrcEnd)
			{
				detail::UTF16Encode(&pDst, *pSrc++);
			}

			return result;
		}

		inline size_t CountCodePoints(const std::string& str) noexcept
		{
			size_t length = 0;

			const char* pSrc = str.c_str();
			const char* const pSrcEnd = pSrc + str.size();

			while (pSrc != pSrcEnd)
			{
				std::int32_t offset;

				detail::utf8_decode(pSrc, pSrcEnd - pSrc, offset);

				pSrc += offset;

				++length;
			}

			return length;
		}

		inline size_t CountCodePoints(const std::u16string& str) noexcept
		{
			return detail::UTF32Length(str);
		}

		inline size_t CountCodePoints(const std::u32string& str) noexcept
		{
			return str.size();
		}

		inline bool Translator_UTF8toUTF32::put(const char code) noexcept
		{
			m_buffer[m_count++] = code;

			const auto result = detail::utf8_decode_check(m_buffer.data(), m_count);

			if (result.offset != -1)
			{
				m_result = result.codePoint;

				m_count = 0;

				return true;
			}

			if (m_count >= 4)
			{
				m_result = 0xFFFD;

				m_count = 0;

				return true;
			}

			return false;
		}

		inline bool Translator_UTF16toUTF32::put(const char16_t code) noexcept
		{
			if (m_hasHighSurrogate)
			{
				m_hasHighSurrogate = false;

				if (IsHighSurrogate(code))
				{
					// error
					m_result = 0xFFFD;
				}
				else if (IsLowSurrogate(code))
				{
					// ok
					m_result = (((m_buffer - 0xD800) << 10) | (code - 0xDC00)) + 0x10000;
				}
				else
				{
					// error
					m_result = 0xFFFD;
				}

				return true;
			}
			else
			{
				if (IsHighSurrogate(code))
				{
					// ok
					m_buffer = code;

					m_hasHighSurrogate = true;

					return false;
				}
				else if (IsLowSurrogate(code))
				{
					// error
					m_result = 0xFFFD;
				}
				else
				{
					// ok
					m_result = code;
				}

				return true;
			}
		}

		inline size_t Translator_UTF32toUTF8::put(const char32_t code) noexcept
		{
			char* pBuffer = m_buffer.data();

			detail::UTF8Encode(&pBuffer, code);

			return pBuffer - m_buffer.data();
		}

		inline size_t Translator_UTF32toUTF16::put(const char32_t code) noexcept
		{
			char16_t* pBuffer = m_buffer.data();

			detail::UTF16Encode(&pBuffer, code);

			return pBuffer - m_buffer.data();
		}
	}
}

